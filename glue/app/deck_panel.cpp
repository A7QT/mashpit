#include "deck_panel.h"

#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "deck_engine.h"

namespace {
QSlider* hslider(int lo, int hi, int value, const QString& tip) {
    auto* s = new QSlider(Qt::Horizontal);
    s->setRange(lo, hi);
    s->setValue(value);
    s->setToolTip(tip);
    return s;
}
} // namespace

DeckPanel::DeckPanel(int deck, DeckEngine* pEngine, const QString& accent, QWidget* pParent)
        : QWidget(pParent), m_deck(deck), m_pEngine(pEngine) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);

    auto* title = new QLabel(deck == 0 ? "DECK A" : "DECK B");
    title->setStyleSheet(QString("font-weight: 800; color: %1;").arg(accent));
    layout->addWidget(title);

    m_pFileLabel = new QLabel(QStringLiteral("— empty —"));
    m_pFileLabel->setWordWrap(true);
    m_pFileLabel->setToolTip("Loaded file. Load accepts WAV / FLAC / OGG / OPUS "
            "(no MP3 decoder in this build config — Phase 1 packaging task).");
    layout->addWidget(m_pFileLabel);

    auto* load = new QPushButton("Load…");
    load->setToolTip("Pick an audio file for this deck (WAV/FLAC/OGG/OPUS)");
    connect(load, &QPushButton::clicked, this, &DeckPanel::onLoad);
    layout->addWidget(load);

    auto* bpmRow = new QHBoxLayout();
    m_pBpmSpin = new QDoubleSpinBox();
    m_pBpmSpin->setRange(60.0, 200.0);
    m_pBpmSpin->setValue(deck == 0 ? 124.0 : 132.0);
    m_pBpmSpin->setDecimals(2);
    m_pBpmSpin->setPrefix("file ");
    m_pBpmSpin->setSuffix(" BPM");
    m_pBpmSpin->setToolTip("File tempo — type it in (analyzer bridge lands in Phase 2), then SYNC");
    bpmRow->addWidget(m_pBpmSpin);
    auto* sync = new QPushButton("SYNC");
    sync->setToolTip("Match to master BPM + keylock on");
    sync->setStyleSheet(QString("font-weight: 800; border: 1px solid %1;").arg(accent));
    connect(sync, &QPushButton::clicked, this, &DeckPanel::onSync);
    bpmRow->addWidget(sync);
    layout->addLayout(bpmRow);

    auto* midRow = new QHBoxLayout();
    m_pRate = new QSlider(Qt::Vertical);
    m_pRate->setRange(92, 108);
    m_pRate->setValue(100);
    m_pRate->setToolTip("Rate slider 92–108% (Mixxx pitch fader). 100 = normal speed");
    connect(m_pRate, &QSlider::valueChanged, this, [this](int v) {
        m_pEngine->setRate(m_deck, v / 100.0);
    });
    midRow->addWidget(m_pRate);

    auto* keyCol = new QVBoxLayout();
    auto* lock = new QPushButton("KEYLOCK");
    lock->setCheckable(true);
    lock->setToolTip("Keylock: speed without pitch change");
    connect(lock, &QPushButton::toggled, this, [this](bool on) {
        m_pEngine->setLock(m_deck, on);
    });
    keyCol->addWidget(lock);
    auto* keyRow = new QHBoxLayout();
    auto* keyDown = new QPushButton("−");
    keyDown->setToolTip("Key down 1 semitone, tempo unchanged (±6 max)");
    auto* keyUp = new QPushButton("+");
    keyUp->setToolTip("Key up 1 semitone, tempo unchanged (±6 max)");
    m_pKeyLabel = new QLabel("+0 st");
    m_pKeyLabel->setAlignment(Qt::AlignCenter);
    m_pKeyLabel->setToolTip("Key shift in semitones (needs keylock ON)");
    connect(keyDown, &QPushButton::clicked, this, [this] { onKey(-1); });
    connect(keyUp, &QPushButton::clicked, this, [this] { onKey(1); });
    keyRow->addWidget(keyDown);
    keyRow->addWidget(m_pKeyLabel);
    keyRow->addWidget(keyUp);
    keyCol->addLayout(keyRow);

    auto* vol = hslider(0, 100, 90, "Deck volume fader");
    connect(vol, &QSlider::valueChanged, this, [this](int v) {
        m_pEngine->setVolume(m_deck, v / 100.0);
    });
    keyCol->addWidget(new QLabel("VOL"));
    keyCol->addWidget(vol);
    midRow->addLayout(keyCol);
    layout->addLayout(midRow);

    auto* eqRow = new QHBoxLayout();
    const char* names[3] = {"LOW", "MID", "HI"};
    const char* tips[3] = {"Low EQ — kill bass to make room", "Mid EQ — vocals live here",
            "High EQ — hats and air"};
    std::vector<QSlider*> eqs;
    for (int i = 0; i < 3; ++i) {
        auto* col = new QVBoxLayout();
        col->addWidget(new QLabel(names[i]));
        auto* s = new QSlider(Qt::Vertical);
        s->setRange(0, 100);
        s->setValue(50);
        s->setToolTip(tips[i]);
        col->addWidget(s);
        eqRow->addLayout(col);
        eqs.push_back(s);
    }
    connect(eqs[0], &QSlider::valueChanged, this, [this, eqs](int) {
        m_pEngine->setEq(m_deck, eqs[0]->value() / 100.0, eqs[1]->value() / 100.0,
                eqs[2]->value() / 100.0);
    });
    connect(eqs[1], &QSlider::valueChanged, this, [this, eqs](int) {
        m_pEngine->setEq(m_deck, eqs[0]->value() / 100.0, eqs[1]->value() / 100.0,
                eqs[2]->value() / 100.0);
    });
    connect(eqs[2], &QSlider::valueChanged, this, [this, eqs](int) {
        m_pEngine->setEq(m_deck, eqs[0]->value() / 100.0, eqs[1]->value() / 100.0,
                eqs[2]->value() / 100.0);
    });
    layout->addLayout(eqRow);
}

void DeckPanel::onLoad() {
    const QString path = QFileDialog::getOpenFileName(this, "Load into deck",
            QDir::homePath() + "/Music", "Audio (WAV FLAC OGG OPUS)(*.wav *.flac *.ogg *.opus)");
    if (path.isEmpty()) {
        return;
    }
    m_pFileLabel->setText(QFileInfo(path).fileName());
    m_pFileLabel->setToolTip(path);
    const QString err = m_pEngine->loadFile(m_deck, path);
    if (!err.isEmpty()) {
        m_pFileLabel->setText(QStringLiteral("LOAD FAILED: ") + QFileInfo(path).fileName());
    }
}

void DeckPanel::onSync() {
    // MainWindow connects syncRequested to the engine with master BPM.
    emit syncRequested(m_deck, m_pBpmSpin->value());
}

void DeckPanel::onKey(int delta) {
    m_key = qBound(-6, m_key + delta, 6);
    m_pKeyLabel->setText(QString("%1%2 st").arg(m_key >= 0 ? "+" : "").arg(m_key));
    m_pEngine->setKey(m_deck, m_key);
}
