// mashpit app v1 — one deck panel. Everything visible is wired to DeckEngine;
// anything not yet backed (hotcues, loops, FX sends, H/M) is OMITTED, not faked.
#pragma once

#include <QWidget>

class DeckEngine;
class JogWidget;
class WaveformWidget;
class QLabel;
class QPushButton;
class QSlider;
class QDoubleSpinBox;

class DeckPanel : public QWidget {
    Q_OBJECT

  public:
    DeckPanel(int deck, DeckEngine* pEngine, const QString& accent, QWidget* pParent = nullptr);

    void setPlayPos(double frac);
    void refreshReadouts();

    // Arrangement/copilot API (all honest, all wired).
    bool loadFilePath(const QString& path); // false = load failed
    void setFileBpm(double bpm);
    void setRatio(double ratio);   // slider + engine
    void setKeySemi(int semis);
    void adjustVolumeDb(double db);
    double fileBpm() const;

  signals:
    void syncRequested(int deck, double fileBpm);

  private:
    void onLoad();
    void onSync();
    void onKey(int delta);
    void onCue();
    void onEq();

    int m_deck;
    DeckEngine* m_pEngine;
    JogWidget* m_pJog;
    WaveformWidget* m_pWave;
    QLabel* m_pFileLabel;
    QLabel* m_pKeyLabel;
    QLabel* m_pRatioLabel;
    QDoubleSpinBox* m_pBpmSpin;
    QSlider* m_pRate;
    QSlider* m_pVol;
    QSlider* m_pEq[3] = {nullptr, nullptr, nullptr};
    int m_key = 0;
};
