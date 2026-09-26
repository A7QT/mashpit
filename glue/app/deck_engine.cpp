#include "deck_engine.h"

#include <QApplication>
#include <QMessageBox>
#include <QTemporaryDir>
#include <cstdio>

#include "control/control.h"
#include "control/controlindicatortimer.h"
#include "control/controlobject.h"
#include "effects/effectsmanager.h"
#include "engine/channelhandle.h"
#include "engine/channels/enginechannel.h"
#include "engine/channels/enginedeck.h"
#include "engine/enginebuffer.h"
#include "mixer/deck.h"
#include "preferences/usersettings.h"
#include "sources/soundsourceproxy.h"
#include "track/track.h"

DeckEngine::DeckEngine() {
    SoundSourceProxy::registerProviders();

    // Scratch settings dir (same pattern as the spike harness).
    static QTemporaryDir settingsDir;
    m_config = UserSettingsPointer(
            new UserSettings(settingsDir.filePath(QStringLiteral("mashpit.cfg"))));
    ControlDoublePrivate::setUserConfig(m_config);

    // Keeps VU/control timers alive (mirrors Mixxx test fixtures).
    static auto indicator = std::make_unique<mixxx::ControlIndicatorTimer>();

    m_pFactory = std::make_shared<ChannelHandleFactory>();
    m_pEffects = new EffectsManager(m_config, m_pFactory);
    m_pMixer = new EngineMixer(m_config, QStringLiteral("[Master]"), m_pEffects,
            m_pFactory, /*bEnableSidechain=*/false);
    ControlObject::set(ConfigKey(QStringLiteral("[Master]"), QStringLiteral("enabled")), 1.0);

    for (int i = 0; i < 2; ++i) {
        m_pDecks[i] = new Deck(nullptr, m_config, m_pMixer, m_pEffects,
                EngineChannel::CENTER, m_pMixer->registerChannelGroup(m_groups[i]));
    }
}

DeckEngine::~DeckEngine() {
    stopAudio();
}

QString DeckEngine::startAudio() {
    if (m_stream != nullptr) {
        return {};
    }
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        return QString::fromLocal8Bit(Pa_GetErrorText(err));
    }
    err = Pa_OpenDefaultStream(&m_stream, 0, 2, paFloat32, kSampleRate,
            kFramesPerBuffer, &DeckEngine::paCallback, this);
    if (err != paNoError) {
        m_stream = nullptr;
        return QString::fromLocal8Bit(Pa_GetErrorText(err));
    }
    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        return QString::fromLocal8Bit(Pa_GetErrorText(err));
    }
    return {};
}

void DeckEngine::stopAudio() {
    if (m_stream == nullptr) {
        return;
    }
    Pa_StopStream(m_stream);
    Pa_CloseStream(m_stream);
    m_stream = nullptr;
    Pa_Terminate();
}

QString DeckEngine::loadFile(int deck, const QString& path) {
    if (deck < 0 || deck > 1) {
        return QStringLiteral("bad deck");
    }
    const QString group = m_groups[deck];
    TrackPointer pTrack = Track::newTemporary(path);
    m_pDecks[deck]->slotLoadTrack(pTrack, false);
    // Pump-then-render contract (docs/SPIKE.md finding 1): the worker only
    // runs while process() pumps the scheduler. With live audio the callback
    // does the pumping; without it, pump manually right here.
    for (int i = 0; i < 200; ++i) {
        if (isLoaded(deck)) {
            return {};
        }
        if (m_stream == nullptr) {
            m_pMixer->process(kFramesPerBuffer * 2); // SAMPLES, not frames
        }
        QThread::msleep(20);
        QApplication::processEvents();
    }
    return QStringLiteral("load timeout (worker never completed)");
}

void DeckEngine::setPlaying(bool play) {
    // One shared transport in v1 (per-deck play lands with DjDeckTrack).
    ControlObject::set(ConfigKey(m_groups[0], QStringLiteral("play")), play ? 1.0 : 0.0);
    ControlObject::set(ConfigKey(m_groups[1], QStringLiteral("play")), play ? 1.0 : 0.0);
}

void DeckEngine::syncDeck(int deck, double masterBpm, double fileBpm) {
    const QString group = m_groups[deck];
    ControlObject::set(ConfigKey(group, QStringLiteral("keylock")), 1.0);
    if (fileBpm > 0) {
        ControlObject::set(ConfigKey(group, QStringLiteral("rate_ratio")),
                masterBpm / fileBpm);
    }
}

void DeckEngine::setLock(int deck, bool on) {
    ControlObject::set(ConfigKey(m_groups[deck], QStringLiteral("keylock")), on ? 1.0 : 0.0);
}

void DeckEngine::setKey(int deck, int semitones) {
    semitones = qBound(-6, semitones, 6);
    ControlObject::set(
            ConfigKey(m_groups[deck], QStringLiteral("pitch_adjust")), double(semitones));
}

void DeckEngine::setRate(int deck, double ratio) {
    ControlObject::set(ConfigKey(m_groups[deck], QStringLiteral("rate_ratio")), ratio);
}

void DeckEngine::setEq(int deck, double low, double mid, double high) {
    // Deck EQ = equalizer effect slot parameters via legacy aliases
    // ([ChannelN],filterLow/Mid/High). Range 0..1, 0.5 = unity, log taper.
    const QString group = m_groups[deck];
    const char* names[3] = {"filterLow", "filterMid", "filterHigh"};
    const double values[3] = {low, mid, high};
    for (int i = 0; i < 3; ++i) {
        ControlObject* co = ControlObject::getControl(
                ConfigKey(group, QString::fromLatin1(names[i])));
        if (co != nullptr) {
            co->set(qBound(0.0, values[i], 1.0));
        } else {
            static bool warned = false;
            if (!warned) {
                warned = true;
                fprintf(stderr, "mashpit: no EQ alias on %s (equalizer chain not set up?)\n",
                        group.toLocal8Bit().constData());
            }
        }
    }
}

void DeckEngine::setVolume(int deck, double vol) {
    ControlObject::set(ConfigKey(m_groups[deck], QStringLiteral("volume")),
            qBound(0.0, vol, 1.0));
}

void DeckEngine::setCrossfader(double v) {
    ControlObject::set(ConfigKey(QStringLiteral("[Master]"), QStringLiteral("crossfader")),
            qBound(-1.0, v, 1.0));
}

void DeckEngine::fireHotcue(int deck, int index, bool alt) {
    // v1: hotcues need EngineBuffer cue plumbing per deck; defer to Phase 3.
    // Kept as a named op so the UI can wire it today and light up later.
    Q_UNUSED(deck);
    Q_UNUSED(index);
    Q_UNUSED(alt);
}

void DeckEngine::setLoop(int deck, int beats) {
    // v1: loop region ops defer to Phase 3 (needs playpos-anchored regions).
    Q_UNUSED(deck);
    Q_UNUSED(beats);
}

void DeckEngine::jumpBeats(int deck, int beats) {
    Q_UNUSED(deck);
    Q_UNUSED(beats);
}

void DeckEngine::cueGo(int deck) {
    Q_UNUSED(deck);
}

void DeckEngine::seekFraction(int deck, double frac) {
    if (deck < 0 || deck > 1) {
        return;
    }
    EngineBuffer* pBuffer = m_pDecks[deck]->getEngineDeck()->getEngineBuffer();
    if (pBuffer == nullptr || !pBuffer->isTrackLoaded()) {
        return;
    }
    const double end = pBuffer->getTrackEndPosition().value();
    if (end > 0) {
        pBuffer->queueNewPlaypos(mixxx::audio::FramePos(
                qBound(0.0, frac, 1.0) * end), EngineBuffer::SEEK_STANDARD);
    }
}

double DeckEngine::playPos(int deck) const {
    return ControlObject::get(
            ConfigKey(m_groups[deck], QStringLiteral("playposition")));
}

double DeckEngine::tempoRatio(int deck) const {
    return ControlObject::get(
            ConfigKey(m_groups[deck], QStringLiteral("rate_ratio")));
}

bool DeckEngine::isLoaded(int deck) const {
    return ControlObject::get(
                   ConfigKey(m_groups[deck], QStringLiteral("track_loaded"))) > 0.5;
}

double DeckEngine::processForTest(int frames) {
    m_pMixer->process(frames * 2); // SAMPLES, not frames
    const CSAMPLE* pMain = m_pMixer->getMainBuffer();
    double peak = 0.0;
    for (int i = 0; i < frames * 2; ++i) {
        const double a = std::fabs(static_cast<double>(pMain[i]));
        if (a > peak) {
            peak = a;
        }
    }
    return peak;
}

int DeckEngine::paCallback(const void*, void* pOutput, unsigned long frames,
        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* pUser) {
    return static_cast<DeckEngine*>(pUser)->render(static_cast<float*>(pOutput), frames);
}

int DeckEngine::render(float* pOut, unsigned long frames) {
    // process() takes SAMPLES (stereo => x2). Spike finding, never again.
    m_pMixer->process(static_cast<int>(frames) * 2);
    const CSAMPLE* pMain = m_pMixer->getMainBuffer();
    double peak = 0.0;
    const unsigned long n = frames * 2;
    for (unsigned long i = 0; i < n; ++i) {
        const float v = pMain[i];
        pOut[i] = v;
        const double a = std::fabs(static_cast<double>(v));
        if (a > peak) {
            peak = a;
        }
    }
    m_peak.store(peak);
    return paContinue;
}
