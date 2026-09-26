#include "deck_panel.h"

#include <QDoubleSpinBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "deck_engine.h"
#include "jog_widget.h"
#include "waveform_widget.h"

DeckPanel::DeckPanel(int deck, DeckEngine* pEngine, const QString& accent, QWidget* pParent)
        : QWidget(pParent), m_deck(deck), m_pEngine(pEngine) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    auto* title = new QLabel(deck == 0 ? "DECK A" : "DECK B");
    title->setStyleSheet(QString("font-weight: 800; color: %1; font-size: 13px;").arg(accent));
    layout->addWidget(title);

    m_pFileLabel = new QLabel(QStringLiteral("— empty —"));
    m_pFileLabel->setWordWrap(true);
    m_pFileLabel->setToolTip("Loaded file (WAV / FLAC / OGG / OPUS — no MP3 decoder in this build)");
    layout->addWidget(m_pFileLabel);

    m_pWave = new WaveformWidget(QColor(accent), this);
    connect(m_pWave, &WaveformWidget::seekClicked, this, [this](double frac) {
        m_pEngine->seekFraction(m_deck, frac);
    });
    layout->addWidget(m_pWave);

    auto* load = new QPushButton(QStringLiteral("Load…"));
    load->setToolTip("Pick an audio file for this deck");
    connect(load, &QPushButton::clicked, this, &DeckPanel::onLoad);
    layout->addWidget(load);

    auto* midRow = new QHBoxLayout();
    midRow->setSpacing(6);
    auto* leftCol = new QVBoxLayout();
    m_pJog = new JogWidget(this);
    connect(m_pJog, &JogWidget::jogged, this, [this](double delta) {
        const double pos = m_pEngine->playPos(m_deck);
        m_pEngine->seekFraction(m_deck, pos + delta);
    });
    leftCol->addWidget(m_pJog);
    auto* cue = new QPushButton("CUE");
    cue->setToolTip("Jump back to the start of the file");
    connect(cue, &QPushButton::clicked, this, &DeckPanel::onCue);
    leftCol->addWidget(cue);
    midRow->addLayout(leftCol);

    auto* ctlCol = new QVBoxLayout();
    ctlCol->setSpacing(4);
    auto* bpmRow = new QHBoxLayout();
    m_pBpmSpin = new QDoubleSpinBox();
    m_pBpmSpin->setRange(60.0, 200.0);
    m_pBpmSpin->setValue(deck == 0 ? 124.0 : 132.0);
    m_pBpmSpin->setDecimals(2);
    m_pBpmSpin->setPrefix("file ");
    m_pBpmSpin->setSuffix(" BPM");
    m_pBpmSpin->setToolTip("File tempo — type it in (analyzer lands in Phase 2), then SYNC");
    bpmRow->addWidget(m_pBpmSpin, 1);
    auto* sync = new QPushButton("SYNC");
    sync->setToolTip("Match to master BPM + keylock on");
    sync->setStyleSheet(QString("font-weight: 800; border: 1px solid %1;").arg(accent));
    connect(sync, &QPushButton::clicked, this, &DeckPanel::onSync);
    bpmRow->addWidget(sync, 1);
    ctlCol->addLayout(bpmRow);

    m_pRatioLabel = new QLabel();
    m_pRatioLabel->setToolTip("Playback speed multiplier and file key");
    ctlCol->addWidget(m_pRatioLabel);
    refreshReadouts();

    auto* keyRow = new QHBoxLayout();
    auto* keyDown = new QPushButton("KEY−");
    keyDown->setToolTip("Key down 1 semitone, tempo unchanged (±6 max)");
    auto* keyUp = new QPushButton("KEY+");
    keyUp->setToolTip("Key up 1 semitone, tempo unchanged (±6 max)");
    m_pKeyLabel = new QLabel("+0 st");
    m_pKeyLabel->setAlignment(Qt::AlignCenter);
    m_pKeyLabel->setToolTip("Key shift in semitones (needs keylock ON)");
    connect(keyDown, &QPushButton::clicked, this, [this] { onKey(-1); });
    connect(keyUp, &QPushButton::clicked, this, [this] { onKey(1); });
    keyRow->addWidget(keyDown);
    keyRow->addWidget(m_pKeyLabel);
    keyRow->addWidget(keyUp);
    ctlCol->addLayout(keyRow);

    auto* volRow = new QHBoxLayout();
    volRow->addWidget(new QLabel("VOL"));
    m_pVol = new QSlider(Qt::Horizontal);
    m_pVol->setRange(0, 100);
    m_pVol->setValue(90);
    m_pVol->setToolTip("Deck volume fader");
    connect(m_pVol, &QSlider::valueChanged, this, [this](int v) {
        m_pEngine->setVolume(m_deck, v / 100.0);
    });
    volRow->addWidget(m_pVol, 1);
    ctlCol->addLayout(volRow);

    auto* eqRow = new QHBoxLayout();
    const char* names[3] = {"LOW", "MID", "HI"};
    const char* tips[3] = {"Low EQ — kill bass to make room", "Mid EQ — vocals live here",
            "High EQ — hats and air"};
    const char* ids[3] = {"low", "mid", "hi"};
    for (int i = 0; i < 3; ++i) {
        auto* col = new QVBoxLayout();
        auto* lab = new QLabel(names[i]);
        lab->setAlignment(Qt::AlignCenter);
        col->addWidget(lab);
        m_pEq[i] = new QSlider(Qt::Horizontal);
        m_pEq[i]->setRange(0, 100);
        m_pEq[i]->setValue(50);
        m_pEq[i]->setToolTip(tips[i]);
        connect(m_pEq[i], &QSlider::valueChanged, this, &DeckPanel::onEq);
        col->addWidget(m_pEq[i]);
        eqRow->addLayout(col);
    }
    ctlCol->addLayout(eqRow);
    midRow->addLayout(ctlCol, 1);

    auto* rightCol = new QVBoxLayout();
    auto* rateLab = new QLabel("+0.00%");
    rateLab->setAlignment(Qt::AlignCenter);
    rateLab->setObjectName("ratepct");
    rightCol->addWidget(rateLab);
    m_pRate = new QSlider(Qt::Vertical);
    m_pRate->setRange(92, 108);
    m_pRate->setValue(100);
    m_pRate->setMaximumHeight(110);
    m_pRate->setToolTip("Rate slider 92–108% (Mixxx pitch fader). 100 = normal speed");
    connect(m_pRate, &QSlider::valueChanged, this, [this, rateLab](int v) {
        m_pEngine->setRate(m_deck, v / 100.0);
        const double pct = v - 100.0;
        rateLab->setText(QString("%1%2%").arg(pct >= 0 ? "+" : "").arg(pct, 0, 'f', 2));
    });
    rightCol->addWidget(m_pRate, 1);
    auto* lock = new QPushButton("KEYLOCK");
    lock->setCheckable(true);
    lock->setToolTip("Keylock: speed without pitch change");
    connect(lock, &QPushButton::toggled, this, [this](bool on) {
        m_pEngine->setLock(m_deck, on);
    });
    rightCol->addWidget(lock);
    midRow->addLayout(rightCol);

    layout->addLayout(midRow);
    layout->addStretch(1);
}

void DeckPanel::setPlayPos(double frac) {
    m_pWave->setPlayPos(frac);
}

void DeckPanel::refreshReadouts() {
    // File keys are Mixxx demo defaults until the analyzer bridge (Phase 2)
    // reads real ones; tempo ratio is live from the engine.
    const QString fileKey = m_deck == 0 ? "8A" : "9A";
    m_pRatioLabel->setText(
            QString("ratio %1 · key %2").arg(m_pEngine->tempoRatio(m_deck), 0, 'f', 4).arg(fileKey));
}

void DeckPanel::onLoad() {
    const QString path = QFileDialog::getOpenFileName(this, "Load into deck",
            QDir::homePath() + "/Music",
            "Audio (WAV FLAC OGG OPUS)(*.wav *.flac *.ogg *.opus)");
    if (path.isEmpty()) {
        return;
    }
    m_pFileLabel->setText(QFileInfo(path).fileName());
    m_pFileLabel->setToolTip(path);
    m_pWave->setFile(path);
    const QString err = m_pEngine->loadFile(m_deck, path);
    if (!err.isEmpty()) {
        m_pFileLabel->setText(QStringLiteral("LOAD FAILED: ") + QFileInfo(path).fileName());
        m_pWave->setFile(QString());
    }
}

void DeckPanel::onSync() {
    emit syncRequested(m_deck, m_pBpmSpin->value());
}

void DeckPanel::onKey(int delta) {
    m_key = qBound(-6, m_key + delta, 6);
    m_pKeyLabel->setText(QString("%1%2 st").arg(m_key >= 0 ? "+" : "").arg(m_key));
    m_pEngine->setKey(m_deck, m_key);
}

void DeckPanel::onCue() {
    m_pEngine->seekFraction(m_deck, 0.0);
}

void DeckPanel::onEq() {
    m_pEngine->setEq(m_deck, m_pEq[0]->value() / 100.0, m_pEq[1]->value() / 100.0,
            m_pEq[2]->value() / 100.0);
}
