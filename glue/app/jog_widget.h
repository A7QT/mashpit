// Jog wheel: drag to seek (vinyl-style). Emits fraction deltas; the panel
// maps them onto DeckEngine::seekFraction from the live play position.
#pragma once

#include <QWidget>

class JogWidget : public QWidget {
    Q_OBJECT

  public:
    explicit JogWidget(QWidget* pParent = nullptr);

  signals:
    void jogged(double deltaFraction);

  protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseMoveEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent*) override;

  private:
    double m_angle = 0.0;
    int m_lastX = 0;
    bool m_drag = false;
};
