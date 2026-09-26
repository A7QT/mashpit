// Copilot dock: suggest-only helper. The user types, the dock proposes ONE
// concrete change with an explanation, and applies it only on Apply.
// Never touches anything by itself. Mirrors `mashpit edit` semantics.
#pragma once

#include <QDockWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class CopilotDock : public QDockWidget {
    Q_OBJECT

  public:
    explicit CopilotDock(QWidget* pParent = nullptr);

  signals:
    // User approved: match all decks to master BPM (file BPMs from spins).
    void applySync();
    // User approved: widen the blend zone (bars) in the open arrangement.
    void applySmooth(double bars);
    // User approved: lift the vocal deck by +db.
    void applyVocalDb(double db);

  private:
    void onAsk();
    void clearSuggestion();

    QLineEdit* m_pPrompt;
    QLabel* m_pSuggestion;
    QPushButton* m_pApply;
    QPushButton* m_pDismiss;
    QString m_pendingKind;
};
