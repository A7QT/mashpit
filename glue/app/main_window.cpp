#include "main_window.h"

#include <QDoubleSpinBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "copilot_dock.h"
#include "deck_engine.h"
#include "deck_panel.h"
#include "timeline_widget.h"

MainWindow::MainWindow(DeckEngine* pEngine, QWidget* pParent)
        : QMainWindow(pParent), m_pEngine(pEngine) {
    setWindowTitle("mashpit 0.1 — booth (app v1)");
    resize(900, 720);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outer = new QVBoxLayout(central);
    auto* top = new QHBoxLayout();

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

    top->addWidget(m_pDecks[0], 1);
    top->addLayout(mid);
    top->addWidget(m_pDecks[1], 1);
    outer->addLayout(top, 1);

    auto* arrRow = new QHBoxLayout();
    auto* openBtn = new QPushButton("Open arrangement…");
    openBtn->setToolTip("Load a mashup.json v1: files go to decks, ratios/keys applied, timeline drawn");
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenArrangement);
    arrRow->addWidget(openBtn);
    arrRow->addWidget(new QLabel("timeline is view-only for now — the booth above is live"));
    outer->addLayout(arrRow);
    m_pTimeline = new TimelineWidget(this);
    outer->addWidget(m_pTimeline);

    auto* dock = new CopilotDock(this);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    connect(dock, &CopilotDock::applySync, this, [this] {
        for (int i = 0; i < 2; ++i) {
            m_pEngine->syncDeck(i, m_pMasterBpm->value(), m_pDecks[i]->fileBpm());
        }
        m_pStatus->setText("Copilot: both decks synced to master.");
    });
    connect(dock, &CopilotDock::applySmooth, this, [this](double bars) {
        if (!m_hasArr) {
            m_pStatus->setText("Copilot: open an arrangement first — nothing to widen.");
            return;
        }
        m_arr.mix.present = true;
        m_arr.mix.bars = bars;
        m_pTimeline->setArrangement(m_arr);
        m_pStatus->setText(QString("Copilot: blend zone widened to %1 bars (view only — live xfader stays yours).").arg(bars));
    });
    connect(dock, &CopilotDock::applyVocalDb, this, [this](double db) {
        // Vocal deck = first clip with role vocal, else deck B.
        int vocal = 1;
        for (int i = 0; i < m_arr.clips.size() && i < 2; ++i) {
            if (m_arr.clips[i].role == "vocal") {
                vocal = i;
            }
        }
        m_pDecks[vocal]->adjustVolumeDb(db);
        m_pStatus->setText(QString("Copilot: deck %1 lifted %2dB.")
                                   .arg(vocal == 0 ? "A" : "B")
                                   .arg(db));
    });

    m_pStatus = new QLabel("Load a file into a deck, hit Play — or Open arrangement.");
    statusBar()->addWidget(m_pStatus);

    m_pVuTimer = new QTimer(this);
    connect(m_pVuTimer, &QTimer::timeout, this, &MainWindow::onVuTick);
    m_pVuTimer->start(50);
}

void MainWindow::onPlayToggle(bool playing) {
    m_pEngine->setPlaying(playing);
    if (playing) {
        m_playClock.start();
    } else {
        m_playBase += m_playClock.isValid() ? m_playClock.elapsed() / 1000.0 : 0.0;
        m_playClock.invalidate();
    }
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (btn != nullptr) {
        btn->setText(playing ? "⏸ Pause" : "▶ Play");
    }
    m_pStatus->setText(playing ? "Playing." : "Stopped.");
}

void MainWindow::onVuTick() {
    const double peak = m_pEngine->peak() * 100.0;
    m_vuShown = qMax(peak, m_vuShown - 4.0);
    const int v = qBound(0, int(m_vuShown), 100);
    m_pVuL->setValue(v);
    m_pVuR->setValue(v);
    const double t = m_playBase +
            (m_playClock.isValid() ? m_playClock.elapsed() / 1000.0 : 0.0);
    for (int i = 0; i < 2; ++i) {
        m_pDecks[i]->setPlayPos(m_pEngine->playPos(i));
        m_pDecks[i]->refreshReadouts();
    }
    if (m_hasArr) {
        m_pTimeline->setPlayheadSec(t);
    }
}

void MainWindow::onOpenArrangement() {
    const QString path = QFileDialog::getOpenFileName(this, "Open arrangement",
            QDir::homePath(), "Mashup JSON (*.json)");
    if (path.isEmpty()) {
        return;
    }
    openArrangementPath(path);
}

void MainWindow::openArrangementPath(const QString& path) {
    QString err;
    Arrangement arr = Arrangement::load(path, &err);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "mashpit — arrangement", err);
        return;
    }
    if (arr.clips.size() > 2) {
        QMessageBox::warning(this, "mashpit — arrangement",
                "v1 opens the first 2 clips (multideck lands with DjDeckTrack).");
    }
    m_pMasterBpm->setValue(arr.bpm);
    for (int i = 0; i < arr.clips.size() && i < 2; ++i) {
        const auto& c = arr.clips[i];
        if (c.file.isEmpty()) {
            QMessageBox::warning(this, "mashpit — arrangement",
                    QString("Clip %1 has no resolvable file, skipped.").arg(c.track));
            continue;
        }
        if (!m_pDecks[i]->loadFilePath(c.file)) {
            QMessageBox::warning(this, "mashpit — arrangement",
                    QString("Could not load %1.").arg(c.file));
            continue;
        }
        // fileBpm = project / ratio, then SYNC machinery does the rest.
        const double fileBpm = arr.bpm / qMax(0.2, c.tempoRatio);
        m_pDecks[i]->setFileBpm(fileBpm);
        m_pEngine->syncDeck(i, arr.bpm, fileBpm);
        m_pDecks[i]->setKeySemi(int(qRound(c.pitchSemi)));
    }
    m_arr = arr;
    m_hasArr = true;
    m_pTimeline->setArrangement(arr);
    m_playBase = 0.0;
    m_playClock.invalidate();
    m_pStatus->setText(QString("Arrangement loaded: %1 (%2 clips @ %3 BPM). Hit Play.")
                               .arg(QFileInfo(path).fileName())
                               .arg(arr.clips.size())
                               .arg(arr.bpm));
}
