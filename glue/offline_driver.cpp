// mashpit OfflineDriver v1 — two-deck offline render on the real Mixxx engine.
//
// Drives EngineMixer with a foreign clock (no SoundManager), honoring the
// Phase-0 spike contracts (docs/SPIKE.md findings):
//   pump-then-render, process() takes SAMPLES, render by playposition.
//
// Usage:
//   offline-driver --a A.wav --b B.wav --out OUT.wav --blocks N --frames F
//     [--a-ratio R --b-ratio R] [--a-gain G --b-gain G] [--a-key K --b-key K]
//     [--b-offset SEC] [--xfade-at SEC --xfade-bars N --bpm B]
//     [--tier preview|full]
//
// --tier preview = SoundTouch keylock engine (fast, deterministic).
// --tier full    = RubberBand Finer R3 (slow, best quality).
// Without --xfade-*, both decks stay CENTER (layered vocal-over-bed style).
// With --xfade-*, A=LEFT and B=RIGHT with an automated crossfader move.
#include <QApplication>
#include <QTemporaryDir>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <sndfile.h>

#include "control/control.h"
#include "control/controlindicatortimer.h"
#include "control/controlobject.h"
#include "effects/effectsmanager.h"
#include "engine/channelhandle.h"
#include "engine/channels/enginechannel.h"
#include "engine/enginebuffer.h"
#include "engine/enginemixer.h"
#include "mixer/deck.h"
#include "preferences/usersettings.h"
#include "sources/soundsourceproxy.h"
#include "track/track.h"
#include "util/sample.h"

namespace {

void say(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

struct Args {
    std::string a;
    std::string b;
    std::string out;
    int blocks = 0;
    int frames = 256;
    double aRatio = 1.0;
    double bRatio = 1.0;
    double aGain = 1.0;
    double bGain = 1.0;
    double aKey = 0.0;
    double bKey = 0.0;
    double bOffset = 0.0;
    double xfadeAt = -1.0;
    double xfadeBars = 0.0;
    double bpm = 128.0;
    bool aOnly = false;
    bool bOnly = false;
    // EngineBuffer::KeylockEngine: SoundTouch=0, RubberBandFaster=1, Finer=2.
    int tier = 0;
};

bool parse(Args* args, int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i];
        auto need = [&](const char* name, std::string* dst) {
            if (i + 1 >= argc) {
                say("missing value for %s", name);
                return false;
            }
            *dst = argv[++i];
            return true;
        };
        std::string val;
        if (flag == "--a") {
            if (!need("--a", &args->a)) return false;
        } else if (flag == "--b") {
            if (!need("--b", &args->b)) return false;
        } else if (flag == "--out") {
            if (!need("--out", &args->out)) return false;
        } else if (flag == "--tier") {
            if (!need("--tier", &val)) return false;
            if (val == "preview") args->tier = 0;
            else if (val == "full") args->tier = 2;
            else { say("tier must be preview|full"); return false; }
        } else if (flag == "--blocks") {
            if (!need("--blocks", &val)) return false;
            args->blocks = atoi(val.c_str());
        } else if (flag == "--frames") {
            if (!need("--frames", &val)) return false;
            args->frames = atoi(val.c_str());
        } else if (flag == "--a-ratio") {
            if (!need("--a-ratio", &val)) return false;
            args->aRatio = atof(val.c_str());
        } else if (flag == "--b-ratio") {
            if (!need("--b-ratio", &val)) return false;
            args->bRatio = atof(val.c_str());
        } else if (flag == "--a-gain") {
            if (!need("--a-gain", &val)) return false;
            args->aGain = atof(val.c_str());
        } else if (flag == "--b-gain") {
            if (!need("--b-gain", &val)) return false;
            args->bGain = atof(val.c_str());
        } else if (flag == "--a-key") {
            if (!need("--a-key", &val)) return false;
            args->aKey = atof(val.c_str());
        } else if (flag == "--b-key") {
            if (!need("--b-key", &val)) return false;
            args->bKey = atof(val.c_str());
        } else if (flag == "--b-offset") {
            if (!need("--b-offset", &val)) return false;
            args->bOffset = atof(val.c_str());
        } else if (flag == "--xfade-at") {
            if (!need("--xfade-at", &val)) return false;
            args->xfadeAt = atof(val.c_str());
        } else if (flag == "--xfade-bars") {
            if (!need("--xfade-bars", &val)) return false;
            args->xfadeBars = atof(val.c_str());
        } else if (flag == "--bpm") {
            if (!need("--bpm", &val)) return false;
            args->bpm = atof(val.c_str());
        } else if (flag == "--a-only") {
            args->aOnly = true;
        } else if (flag == "--b-only") {
            args->bOnly = true;
        } else {
            say("unknown flag %s", flag.c_str());
            return false;
        }
    }
    if (args->a.empty() || args->b.empty() || args->out.empty() ||
            args->blocks <= 0 || args->frames <= 0 || args->frames % 2 != 0) {
        say("need --a --b --out --blocks N --frames F(even)");
        return false;
    }
    if (args->aGain < 0.0 || args->aGain > 1.0 || args->bGain < 0.0 || args->bGain > 1.0) {
        say("gains are channel volume 0..1 (boosts happen in post, not here)");
        return false;
    }
    return true;
}

const QString kMasterGroup = QStringLiteral("[Master]");
const QString kDeckA = QStringLiteral("[Channel1]");
const QString kDeckB = QStringLiteral("[Channel2]");

void setDeck(const QString& deck, const std::string& file, double ratio,
        double gain, double key, Deck* pDeck) {
    TrackPointer pTrack = Track::newTemporary(QString::fromLocal8Bit(file.c_str()));
    pDeck->slotLoadTrack(pTrack, false);
    ControlObject::set(ConfigKey(deck, QStringLiteral("keylock")), 1.0);
    ControlObject::set(ConfigKey(deck, QStringLiteral("rate_ratio")), ratio);
    ControlObject::set(ConfigKey(deck, QStringLiteral("volume")), gain);
    ControlObject::set(ConfigKey(deck, QStringLiteral("pitch_adjust")), key);
}

bool pumpUntilLoaded(Deck* pDeck, const QString& deck, EngineMixer* pMixer, int frames) {
    int spin = 0;
    while (ControlObject::get(ConfigKey(deck, QStringLiteral("track_loaded"))) < 0.5 &&
            spin < 4000) {
        pMixer->process(frames * 2); // process() takes SAMPLES (stereo => x2)
        ++spin;
    }
    if (spin >= 4000) {
        say("LOAD TIMEOUT on %s", deck.toLocal8Bit().constData());
        return false;
    }
    say("%s loaded after %d pump blocks", deck.toLocal8Bit().constData(), spin);
    return true;
}

// Same trick as Mixxx's SignalPathTest: force the outputs on without audio HW.
class TestEngineMixer : public EngineMixer {
  public:
    TestEngineMixer(UserSettingsPointer config,
            const QString& group,
            EffectsManager* pEffectsManager,
            ChannelHandleFactoryPointer pChannelHandleFactory)
            : EngineMixer(config, group, pEffectsManager, pChannelHandleFactory, false) {
        m_pMainEnabled->forceSet(1);
        m_pHeadphoneEnabled->forceSet(1);
        m_pBoothEnabled->forceSet(1);
    }
};

} // namespace

int main(int argc, char** argv) {
    Args args;
    if (!parse(&args, argc, argv)) {
        return 2;
    }
    QApplication app(argc, argv);

    if (!SoundSourceProxy::registerProviders()) {
        say("no SoundSource providers registered");
        return 3;
    }

    QTemporaryDir settingsDir;
    UserSettingsPointer config(
            new UserSettings(settingsDir.filePath(QStringLiteral("spike.cfg"))));
    ControlDoublePrivate::setUserConfig(config);

    auto indicator = std::make_unique<mixxx::ControlIndicatorTimer>();
    auto factory = std::make_shared<ChannelHandleFactory>();
    auto* pEffects = new EffectsManager(config, factory);
    TestEngineMixer mixer(config, kMasterGroup, pEffects, factory);

    // Keylock engine tier: the CO may not exist outside full Mixxx prefs,
    // so create it (ControlProxies in each EngineBuffer resolve by key).
    auto* pKeylockEngine =
            new ControlObject(ConfigKey(QStringLiteral("[App]"), QStringLiteral("keylock_engine")));
    pKeylockEngine->set(static_cast<double>(args.tier));
    say("keylock engine tier %d (%s)", args.tier, args.tier == 2 ? "RubberBandFiner" : "SoundTouch");

    const bool doXfade = args.xfadeBars > 0 && args.xfadeAt >= 0;
    Deck* pDeckA = new Deck(nullptr, config, &mixer, pEffects,
            doXfade ? EngineChannel::LEFT : EngineChannel::CENTER,
            mixer.registerChannelGroup(kDeckA));
    Deck* pDeckB = new Deck(nullptr, config, &mixer, pEffects,
            doXfade ? EngineChannel::RIGHT : EngineChannel::CENTER,
            mixer.registerChannelGroup(kDeckB));

    setDeck(kDeckA, args.a, args.aRatio, args.aGain, args.aKey, pDeckA);
    setDeck(kDeckB, args.b, args.bRatio, args.bGain, args.bKey, pDeckB);
    if (!pumpUntilLoaded(pDeckA, kDeckA, &mixer, args.frames) ||
            !pumpUntilLoaded(pDeckB, kDeckB, &mixer, args.frames)) {
        return 6;
    }

    const double xfadeSecs = args.xfadeBars * (60.0 / args.bpm) * 4.0;
    ControlObject::set(ConfigKey(kDeckA, QStringLiteral("play")), 1.0);
    if (args.bOffset <= 0) {
        ControlObject::set(ConfigKey(kDeckB, QStringLiteral("play")), 1.0);
    }
    if (doXfade) {
        ControlObject::set(ConfigKey(kMasterGroup, QStringLiteral("crossfader")), -1.0);
    }

    SF_INFO sf{};
    sf.samplerate = 44100; // EngineMixer default
    sf.channels = 2;
    sf.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE* out = sf_open(args.out.c_str(), SFM_WRITE, &sf);
    if (out == nullptr) {
        say("cannot open output wav");
        return 3;
    }

    const ConfigKey kPlayB(kDeckB, QStringLiteral("play"));
    const ConfigKey kXfader(kMasterGroup, QStringLiteral("crossfader"));
    if (args.aOnly) {
        ControlObject::set(ConfigKey(kDeckB, QStringLiteral("mute")), 1.0);
        say("solo: deck A only");
    }
    if (args.bOnly) {
        ControlObject::set(ConfigKey(kDeckA, QStringLiteral("mute")), 1.0);
        say("solo: deck B only");
    }
    double peak = 0.0;
    double mixTime = 0.0;
    bool bStarted = args.bOffset <= 0;
    for (int i = 0; i < args.blocks; ++i) {
        if (!bStarted && mixTime >= args.bOffset) {
            ControlObject::set(kPlayB, 1.0);
            bStarted = true;
            say("deck B started @ %.2fs", mixTime);
        }
        if (doXfade) {
            double x = (mixTime - args.xfadeAt) / xfadeSecs;
            x = x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x);
            ControlObject::set(kXfader, -1.0 + 2.0 * x);
        }
        mixer.process(args.frames * 2); // SAMPLES, not frames
        const CSAMPLE* pMain = mixer.getMainBuffer();
        for (int s = 0; s < args.frames * 2; ++s) {
            const double v = std::fabs(static_cast<double>(pMain[s]));
            if (v > peak) {
                peak = v;
            }
        }
        sf_write_float(out, pMain, args.frames * 2);
        mixTime += static_cast<double>(args.frames) / 44100.0;
        if (i % 2000 == 0) {
            say("block %d t=%.1fs peak=%.3f", i, mixTime, peak);
        }
    }
    sf_close(out);
    say("done t=%.1fs peak=%.3f -> %s", mixTime, peak, args.out.c_str());
    if (peak < 1e-6) {
        say("SILENCE: rendered all zeros");
        return 5;
    }
    return 0;
}
