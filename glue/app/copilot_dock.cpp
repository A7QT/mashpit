#include "copilot_dock.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

CopilotDock::CopilotDock(QWidget* pParent) : QDockWidget("Copilot (suggest-only)", pParent) {
    auto* body = new QWidget(this);
    auto* layout = new QVBoxLayout(body);
    auto* hint = new QLabel("I suggest, you decide. Nothing applies itself.");
    hint->setWordWrap(true);
    layout->addWidget(hint);
    m_pPrompt = new QLineEdit("transition feels abrupt");
    m_pPrompt->setToolTip("Try: 'match bpm' · 'smooth the transition' · 'vocal sits better?'");
    layout->addWidget(m_pPrompt);
    auto* ask = new QPushButton("Ask");
    ask->setToolTip("Get a suggestion (nothing changes yet)");
    connect(ask, &QPushButton::clicked, this, &CopilotDock::onAsk);
    layout->addWidget(ask);
    m_pSuggestion = new QLabel("suggestion appears here…");
    m_pSuggestion->setWordWrap(true);
    m_pSuggestion->setMinimumHeight(70);
    layout->addWidget(m_pSuggestion);
    auto* row = new QHBoxLayout();
    m_pApply = new QPushButton("Apply");
    m_pApply->setEnabled(false);
    m_pDismiss = new QPushButton("Dismiss");
    m_pDismiss->setEnabled(false);
    connect(m_pApply, &QPushButton::clicked, this, [this] {
        if (m_pendingKind == "sync") {
            emit applySync();
        } else if (m_pendingKind == "smooth") {
            emit applySmooth(8.0);
        } else if (m_pendingKind == "vocal") {
            emit applyVocalDb(1.5);
        }
        clearSuggestion();
    });
    connect(m_pDismiss, &QPushButton::clicked, this, &CopilotDock::clearSuggestion);
    row->addWidget(m_pApply);
    row->addWidget(m_pDismiss);
    layout->addLayout(row);
    layout->addStretch(1);
    setWidget(body);
}

void CopilotDock::onAsk() {
    const QString p = m_pPrompt->text().toLower();
    if (p.contains("bpm") || p.contains("match") || p.contains("sync")) {
        m_pSuggestion->setText("Suggestion: SYNC both decks to master (file BPMs from the spins) + keylock on.");
        m_pendingKind = "sync";
    } else if (p.contains("abrupt") || p.contains("smooth") || p.contains("transition") ||
            p.contains("0:21")) {
        m_pSuggestion->setText("Suggestion: widen the blend zone to 8 bars (timeline view updates; "
                               "your live crossfader stays yours).");
        m_pendingKind = "smooth";
    } else if (p.contains("vocal") || p.contains("sit") || p.contains("louder") ||
            p.contains("gain")) {
        m_pSuggestion->setText("Suggestion: lift the vocal deck +1.5dB.");
        m_pendingKind = "vocal";
    } else {
        m_pSuggestion->setText("I do bpm, blends and vocal balance — try 'transition feels abrupt'.");
        m_pendingKind.clear();
        m_pApply->setEnabled(false);
        m_pDismiss->setEnabled(false);
        return;
    }
    m_pApply->setEnabled(true);
    m_pDismiss->setEnabled(true);
}

void CopilotDock::clearSuggestion() {
    m_pSuggestion->setText("dismissed — nothing changed.");
    m_pendingKind.clear();
    m_pApply->setEnabled(false);
    m_pDismiss->setEnabled(false);
}
