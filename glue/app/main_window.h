// mashpit app v1 — main window: two deck panels, transport, crossfader, VU.
#pragma once

#include <QMainWindow>

class DeckEngine;
class DeckPanel;
class QLabel;
class QProgressBar;
class QDoubleSpinBox;
class QSlider;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(DeckEngine* pEngine, QWidget* pParent = nullptr);

  private:
    void onPlayToggle(bool playing);
    void onVuTick();

    DeckEngine* m_pEngine;
    DeckPanel* m_pDecks[2];
    QDoubleSpinBox* m_pMasterBpm;
    QSlider* m_pXfader;
    QProgressBar* m_pVuL;
    QProgressBar* m_pVuR;
    QLabel* m_pStatus;
    QTimer* m_pVuTimer;
    double m_vuShown = 0.0;
};
