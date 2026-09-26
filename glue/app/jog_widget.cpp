#include "jog_widget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

JogWidget::JogWidget(QWidget* pParent) : QWidget(pParent) {
    setFixedSize(76, 76);
    setCursor(Qt::OpenHandCursor);
    setToolTip("Jog: drag to seek through the track (vinyl-style)");
}

void JogWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPointF c(width() / 2.0, height() / 2.0);
    p.setBrush(QColor("#23262f"));
    p.setPen(QPen(QColor("#454b5e"), 2));
    p.drawEllipse(c, 34, 34);
    p.setBrush(QColor("#343947"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, 12, 12);
    p.save();
    p.translate(c);
    p.rotate(m_angle);
    p.setBrush(QColor("#e8eaf0"));
    p.drawRoundedRect(-1.5, -32, 3, 15, 1, 1);
    p.restore();
}

void JogWidget::mousePressEvent(QMouseEvent* pEvent) {
    m_drag = true;
    m_lastX = pEvent->pos().x();
    setCursor(Qt::ClosedHandCursor);
}

void JogWidget::mouseMoveEvent(QMouseEvent* pEvent) {
    if (!m_drag) {
        return;
    }
    const int dx = pEvent->pos().x() - m_lastX;
    m_lastX = pEvent->pos().x();
    m_angle += dx * 0.8;
    update();
    emit jogged(dx / 400.0); // full-width drag ~= quarter of the file
}

void JogWidget::mouseReleaseEvent(QMouseEvent*) {
    m_drag = false;
    setCursor(Qt::OpenHandCursor);
}
