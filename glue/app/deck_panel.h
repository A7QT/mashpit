// mashpit app v1 — one deck panel. Everything here is wired to DeckEngine;
// anything not yet backed (hotcues, loops, FX sends) is OMITTED, not faked.
#pragma once

#include <QWidget>

class DeckEngine;
class QLabel;
class QPushButton;
class QSlider;
class QDoubleSpinBox;

class DeckPanel : public QWidget {
    Q_OBJECT

  public:
    DeckPanel(int deck, DeckEngine* pEngine, const QString& accent, QWidget* pParent = nullptr);

  private:
    void onLoad();
    void onSync();
    void onKey(int delta);

    int m_deck;
    DeckEngine* m_pEngine;
    QLabel* m_pFileLabel;
    QLabel* m_pKeyLabel;
    QDoubleSpinBox* m_pBpmSpin;
    QSlider* m_pRate;
    int m_key = 0;

  signals:
    void syncRequested(int deck, double fileBpm);
};
