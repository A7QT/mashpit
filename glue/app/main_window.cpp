#include "main_window.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "deck_engine.h"
#include "deck_panel.h"

MainWindow::MainWindow(DeckEngine* pEngine, QWidget* pParent)
        : QMainWindow(pParent), m_pEngine(pEngine) {
    setWindowTitle("mashpit 0.1 — booth (app v1)");
    resize(860, 560);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* top = new QHBoxLayout(central);

    m_pDecks[0] = new DeckPanel(0, pEngine, "#5b9dff");
    m_pDecks[1] = new DeckPanel(1, pEngine, "#4ed07e");
    for (int i = 0; i < 2; ++i) {
        connect(m_pDecks[i], &DeckPanel::syncRequested, this,
                [this](int deck, double fileBpm) {
                    m_pEngine->syncDeck(deck, m_pMasterBpm->value(), fileBpm);
                });
    }

    auto* mid = new QVBoxLayout();
    mid->addWidget(new QLabel("TRANSPORT"));
    auto* play = new QPushButton("▶ Play");
    play->setCheckable(true);
    play->setToolTip("Shared transport (per-deck play lands with DjDeckTrack)");
    play->setStyleSheet("font-weight: 800; font-size: 14px; padding: 8px;");
    connect(play, &QPushButton::toggled, this, &MainWindow::onPlayToggle);
    mid->addWidget(play);

    auto* bpmRow = new QHBoxLayout();
    bpmRow->addWidget(new QLabel("Master"));
    m_pMasterBpm = new QDoubleSpinBox();
    m_pMasterBpm->setRange(60.0, 200.0);
    m_pMasterBpm->setValue(128.0);
    m_pMasterBpm->setSuffix(" BPM");
    m_pMasterBpm->setToolTip("Master tempo — SYNC targets this");
    bpmRow->addWidget(m_pMasterBpm);
    mid->addLayout(bpmRow);

    mid->addWidget(new QLabel("XFADE"));
    m_pXfader = new QSlider(Qt::Horizontal);
    m_pXfader->setRange(0, 100);
    m_pXfader->setValue(50);
    m_pXfader->setToolTip("Crossfader: left = A only, middle = both, right = B only");
    connect(m_pXfader, &QSlider::valueChanged, this, [this](int v) {
        m_pEngine->setCrossfader(v / 50.0 - 1.0);
    });
    mid->addWidget(m_pXfader);

    mid->addWidget(new QLabel("MASTER"));
    m_pVuL = new QProgressBar();
    m_pVuR = new QProgressBar();
    for (auto* vu : {m_pVuL, m_pVuR}) {
        vu->setRange(0, 100);
        vu->setValue(0);
        vu->setTextVisible(false);
        vu->setToolTip("Master output peak (measured in the audio callback)");
        mid->addWidget(vu);
    }
    mid->addStretch(1);
    auto* note = new QLabel("v1: booth plays + blends.\nRender stays in\n`mashpit render`.");
    note->setWordWrap(true);
    mid->addWidget(note);

    top->addWidget(m_pDecks[0], 1);
    top->addLayout(mid);
    top->addWidget(m_pDecks[1], 1);

    m_pStatus = new QLabel("Load a file into a deck, hit Play.");
    statusBar()->addWidget(m_pStatus);

    m_pVuTimer = new QTimer(this);
    connect(m_pVuTimer, &QTimer::timeout, this, &MainWindow::onVuTick);
    m_pVuTimer->start(50);
}

void MainWindow::onPlayToggle(bool playing) {
    m_pEngine->setPlaying(playing);
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (btn != nullptr) {
        btn->setText(playing ? "⏸ Pause" : "▶ Play");
    }
    m_pStatus->setText(playing ? "Playing." : "Stopped.");
}

void MainWindow::onVuTick() {
    // Cheap ballistics: fast attack from the callback peak, slow release.
    const double peak = m_pEngine->peak() * 100.0;
    m_vuShown = qMax(peak, m_vuShown - 4.0);
    const int v = qBound(0, int(m_vuShown), 100);
    m_pVuL->setValue(v);
    m_pVuR->setValue(v);
}
