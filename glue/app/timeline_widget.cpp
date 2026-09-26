#include "timeline_widget.h"

#include <QPainter>

TimelineWidget::TimelineWidget(QWidget* pParent) : QWidget(pParent) {
    setMinimumHeight(120);
    setMaximumHeight(160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip("Arrangement timeline: clips on the bar grid, gold = blend zone. "
               "Load via Open arrangement below.");
}

void TimelineWidget::setArrangement(const Arrangement& arrangement) {
    m_arr = arrangement;
    m_hasArr = true;
    m_playhead = -1.0;
    update();
}

void TimelineWidget::setPlayheadSec(double seconds) {
    m_playhead = seconds;
    update();
}

void TimelineWidget::clear() {
    m_hasArr = false;
    m_playhead = -1.0;
    update();
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#14161c"));
    if (!m_hasArr || m_arr.clips.isEmpty()) {
        p.setPen(QColor("#5a6376"));
        p.drawText(rect(), Qt::AlignCenter,
                "no arrangement — Open arrangement below (mashup.json v1)");
        return;
    }
    const double total = m_arr.totalSec();
    const double x0 = 8.0, x1 = width() - 8.0;
    const auto X = [&](double sec) { return x0 + (sec / total) * (x1 - x0); };
    // bar grid
    p.setPen(QColor(255, 255, 255, 22));
    const double spb = m_arr.secPerBar();
    for (double s = 0; s <= total; s += spb) {
        const int x = int(X(s));
        p.drawLine(x, 18, x, height() - 6);
    }
    // ruler (adaptive density so labels never collide)
    p.setPen(QColor("#8b93a5"));
    const double totalBars = total / spb;
    const int labelEvery = totalBars > 48 ? 8 : 4;
    for (double b = 0; b <= totalBars; b += labelEvery) {
        const double s = b * spb;
        p.drawText(int(X(s)) + 2, 12, QString("bar %1").arg(int(b) + 1));
    }
    // xfade zone
    if (m_arr.mix.present) {
        const double s0 = (m_arr.mix.atBar - 1.0) * spb;
        p.fillRect(QRectF(X(s0), 18, m_arr.mix.bars * spb / total * (x1 - x0), height() - 24),
                QColor(224, 161, 0, 26));
        p.setPen(QColor("#e0a100"));
        p.drawText(int(X(s0)) + 3, height() - 10,
                QString("blend %1 bars").arg(m_arr.mix.bars));
    }
    // clips (two lanes)
    const QColor laneCols[2] = {QColor("#5b9dff"), QColor("#4ed07e")};
    for (int i = 0; i < m_arr.clips.size() && i < 4; ++i) {
        const auto& c = m_arr.clips[i];
        const int lane = i % 2;
        const int y = 24 + lane * ((height() - 34) / 2);
        const int h = (height() - 34) / 2 - 4;
        QColor col = laneCols[lane];
        col.setAlpha(70);
        p.fillRect(QRectF(X(m_arr.clipStartSec(c)), y,
                          c.bars * spb / total * (x1 - x0), h),
                col);
        p.setPen(laneCols[lane]);
        p.drawText(int(X(m_arr.clipStartSec(c))) + 4, y + 13, c.track);
    }
    // playhead
    if (m_playhead >= 0) {
        p.setPen(QColor("#e05252"));
        const int px = int(X(m_playhead));
        p.drawLine(px, 16, px, height() - 4);
    }
}
