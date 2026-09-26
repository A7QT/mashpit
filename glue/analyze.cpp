// mashpit analyzer bridge v1 — Mixxx analysis stack as a CLI sidecar writer.
//
//   mashpit-analyze <audiofile> [--out sidecar.json]
//
// Runs the REAL Mixxx analyzers (QueenMary beats default, KeyFinder/QueenMary
// key, ReplayGain) by mirroring AnalyzerThread's feed loop, then writes the
// *.analysis.json sidecar the copilot digs through (docs/ARCHITECTURE.md).
// No DB, no GUI, no SoundManager. Exit non-zero on failure.
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <cstdarg>
#include <cstdio>

#include "analyzer/analyzerbeats.h"
#include "analyzer/analyzerebur128.h"
#include "analyzer/analyzerkey.h"
#include "analyzer/analyzertrack.h"
#include "analyzer/constants.h"
#include "control/control.h"
#include "preferences/keydetectionsettings.h"
#include "preferences/usersettings.h"
#include "sources/audiosourcestereoproxy.h"
#include "sources/soundsourceproxy.h"
#include "track/keyutils.h"
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

QString escapeJson(const QString& s) {
    QString o = s;
    o.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n");
    return o;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        say("usage: mashpit-analyze <audiofile> [--out sidecar.json]");
        return 2;
    }
    const QString inFile = QString::fromLocal8Bit(argv[1]);
    QString outFile;
    for (int i = 2; i < argc; ++i) {
        if (QString(argv[i]) == "--out" && i + 1 < argc) {
            outFile = QString::fromLocal8Bit(argv[++i]);
        }
    }

    QCoreApplication app(argc, argv);

    if (!SoundSourceProxy::registerProviders()) {
        say("no SoundSource providers registered");
        return 3;
    }

    QTemporaryDir settingsDir;
    UserSettingsPointer config(
            new UserSettings(settingsDir.filePath(QStringLiteral("mashpit.cfg"))));
    ControlDoublePrivate::setUserConfig(config);

    TrackPointer pTrack = Track::newTemporary(inFile);
    AnalyzerTrack track(pTrack);

    mixxx::AudioSource::OpenParams openParams;
    openParams.setChannelCount(mixxx::kAnalysisChannels);
    const mixxx::AudioSourcePointer pSource =
            SoundSourceProxy(pTrack).openAudioSource(openParams);
    if (!pSource) {
        say("cannot open audio file (no decoder; MP3 needs MAD/FFmpeg dev libs)");
        return 3;
    }
    pTrack->setAudioProperties(pSource->getStreamInfo());

    AnalyzerBeats beats(config, /*enforceBpmDetection=*/true);
    AnalyzerKey key{KeyDetectionSettings(config)};
    AnalyzerEbur128 gain(config);

    const auto sampleRate = pSource->getSignalInfo().getSampleRate();
    const auto frameLength = pSource->frameLength();
    bool wantBeats = beats.initialize(track, sampleRate, frameLength);
    bool wantKey = key.initialize(track, sampleRate, frameLength);
    bool wantGain = gain.initialize(track, sampleRate, frameLength);
    if (!wantBeats && !wantKey && !wantGain) {
        say("no analyzer initialized (check file + prefs)");
        return 4;
    }

    mixxx::AudioSourceStereoProxy proxy(pSource, mixxx::kAnalysisFramesPerChunk);
    mixxx::SampleBuffer sampleBuffer(mixxx::kAnalysisSamplesPerChunk);
    mixxx::IndexRange remaining = pSource->frameIndexRange();
    while (!remaining.empty()) {
        auto chunk = remaining.splitAndShrinkFront(
                std::min<SINT>(mixxx::kAnalysisFramesPerChunk, remaining.length()));
        const auto readable = proxy.readSampleFrames(mixxx::WritableSampleFrames(
                chunk, mixxx::SampleBuffer::WritableSlice(sampleBuffer)));
        const SINT nSamples = readable.frameIndexRange().length() *
                mixxx::kAnalysisChannels;
        if (wantBeats) {
            beats.processSamples(sampleBuffer.data(), nSamples);
        }
        if (wantKey) {
            key.processSamples(sampleBuffer.data(), nSamples);
        }
        if (wantGain) {
            gain.processSamples(sampleBuffer.data(), nSamples);
        }
    }
    if (wantBeats) {
        beats.storeResults(pTrack);
        beats.cleanup();
    }
    if (wantKey) {
        key.storeResults(pTrack);
        key.cleanup();
    }
    if (wantGain) {
        gain.storeResults(pTrack);
        gain.cleanup();
    }

    const double bpm = pTrack->getBpm();
    const auto chromaticKey = pTrack->getKey();
    const QString keyText =
            KeyUtils::keyToString(chromaticKey, KeyUtils::KeyNotation::Lancelot);
    const double gainDb = 20.0 * std::log10(pTrack->getReplayGain().getRatio() + 1e-12);
    const double durationSec = pTrack->getDuration();
    const int sections = static_cast<int>(
            pTrack->getBeats() ? pTrack->getBeats()->getMarkers().size() : 0);

    QString json = QString(
            "{\n"
            "  \"file\": \"%1\",\n"
            "  \"bpm\": %2,\n"
            "  \"key\": \"%3\",\n"
            "  \"replayGainDb\": %4,\n"
            "  \"durationSec\": %5,\n"
            "  \"sampleRate\": %6,\n"
            "  \"channels\": %7,\n"
            "  \"beatSections\": %8\n"
            "}\n")
                           .arg(escapeJson(inFile))
                           .arg(bpm, 0, 'f', 2)
                           .arg(escapeJson(keyText))
                           .arg(gainDb, 0, 'f', 1)
                           .arg(durationSec, 0, 'f', 1)
                           .arg(pSource->getSignalInfo().getSampleRate().value())
                           .arg(static_cast<int>(
                                   pSource->getSignalInfo().getChannelCount()))
                           .arg(sections);
    if (outFile.isEmpty()) {
        fwrite(json.toUtf8().constData(), 1, json.toUtf8().size(), stdout);
    } else {
        FILE* f = fopen(outFile.toLocal8Bit().constData(), "w");
        if (f == nullptr) {
            say("cannot write sidecar");
            return 3;
        }
        fwrite(json.toUtf8().constData(), 1, json.toUtf8().size(), f);
        fclose(f);
    }
    say("bpm=%.2f key=%s gain=%+.1fdB dur=%.1fs sections=%d",
            bpm, keyText.toLocal8Bit().constData(), gainDb, durationSec, sections);
    return 0;
}
