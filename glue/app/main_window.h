// mashpit app v1 — main window: booth, arrangement timeline, copilot dock.
#pragma once

#include <QElapsedTimer>
#include <QMainWindow>

#include "arrangement.h"

class DeckEngine;
class DeckPanel;
class TimelineWidget;
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
    void onOpenArrangement();

  public:
    // Headless-testable entry: same path as the dialog, no GUI blocking.
    void openArrangementPath(const QString& path);

    DeckEngine* m_pEngine;
    DeckPanel* m_pDecks[2];
    TimelineWidget* m_pTimeline;
    QDoubleSpinBox* m_pMasterBpm;
    QSlider* m_pXfader;
    QProgressBar* m_pVuL;
    QProgressBar* m_pVuR;
    QLabel* m_pStatus;
    QTimer* m_pVuTimer;
    Arrangement m_arr;
    bool m_hasArr = false;
    QElapsedTimer m_playClock;
    double m_playBase = 0.0; // seconds banked before the current run
    double m_vuShown = 0.0;
};
