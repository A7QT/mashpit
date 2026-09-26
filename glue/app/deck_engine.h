// mashpit app v1 engine: two real Mixxx decks + EngineMixer, clocked by a
// PortAudio callback (live) instead of SoundManager. Same classes as the
// Phase-0 harness; same contracts (docs/SPIKE.md). GUI thread drives
// ControlObjects exactly like Mixxx skins do — that path is thread-safe
// by Mixxx design.
//
// v1 limits: WAV/FLAC/OGG/OPUS only (no MP3 decoder in this build config),
// file BPM typed by hand (analyzer bridge = Phase 2), no timeline
// (DjDeckTrack = later). Render stays in `mashpit render` (offline-driver).
#pragma once

#include <QString>
#include <atomic>
#include <memory>

#include <portaudio.h>

#include "control/controlobject.h"
#include "engine/enginemixer.h"
#include "preferences/usersettings.h"
#include "track/track.h"

class ChannelHandleFactory;
class Deck;
class EffectsManager;

class DeckEngine : public QObject {
    Q_OBJECT

  public:
    static constexpr int kSampleRate = 44100;
    static constexpr int kFramesPerBuffer = 512;

    DeckEngine();
    ~DeckEngine() override;

    // Starts the audio stream (silence until tracks load). Returns error
    // text on failure, empty string on success.
    QString startAudio();
    void stopAudio();
    bool audioRunning() const {
        return m_stream != nullptr;
    }

    // Blocking (up to ~4s): loads file into deck 0/1 via the worker path,
    // pumping on load completion like the OfflineDriver contract demands.
    // Returns error text, empty on success.
    QString loadFile(int deck, const QString& path);

    void setPlaying(bool play);
    void syncDeck(int deck, double masterBpm, double fileBpm);
    void setLock(int deck, bool on);
    void setKey(int deck, int semitones);
    void setRate(int deck, double ratio);
    void setEq(int deck, double low, double mid, double high);
    void setVolume(int deck, double vol);   // channel volume 0..1
    void setCrossfader(double v);           // -1 (A) .. +1 (B)
    void fireHotcue(int deck, int index, bool alt);
    void setLoop(int deck, int beats);      // 0 = release
    void jumpBeats(int deck, int beats);
    void cueGo(int deck);

    double peak() const {
        return m_peak.load();
    }

    // Test/robustness hooks (also used when no audio device is open: the
    // caller pumps the engine manually, same contract as OfflineDriver).
    bool isLoaded(int deck) const;
    double processForTest(int frames);

  private:
    static int paCallback(const void* pInput, void* pOutput,
            unsigned long frames, const PaStreamCallbackTimeInfo*,
            PaStreamCallbackFlags, void* pUser);

    int render(float* pOut, unsigned long frames);

    UserSettingsPointer m_config;
    std::shared_ptr<ChannelHandleFactory> m_pFactory;
    EffectsManager* m_pEffects = nullptr;
    EngineMixer* m_pMixer = nullptr;
    Deck* m_pDecks[2] = {nullptr, nullptr};
    QString m_groups[2] = {QStringLiteral("[Channel1]"), QStringLiteral("[Channel2]")};
    PaStream* m_stream = nullptr;
    std::atomic<double> m_peak{0.0};
};
