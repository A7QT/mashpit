#include "waveform_widget.h"

#include <QMouseEvent>
#include <QPainter>

#include <sndfile.h>

namespace {
constexpr int kBuckets = 500;
} // namespace

WaveformWidget::WaveformWidget(const QColor& accent, QWidget* pParent)
        : QWidget(pParent), m_accent(accent) {
    setMinimumHeight(64);
    setMaximumHeight(96);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip("Waveform (peaks from file). Click to seek.");
}

void WaveformWidget::setFile(const QString& path) {
    m_peaks.clear();
    m_haveFile = false;
    m_playPos = 0.0;
    if (path.isEmpty()) {
        update();
        return;
    }
    SF_INFO info{};
    SNDFILE* f = sf_open(path.toLocal8Bit().constData(), SFM_READ, &info);
    if (f == nullptr || info.frames <= 0) {
        if (f != nullptr) {
            sf_close(f);
        }
        update();
        return; // unsupported format (e.g. opus/m4a): placeholder stays
    }
    const sf_count_t per = info.frames / kBuckets + 1;
    std::vector<float> buf(static_cast<size_t>(per) * info.channels);
    m_peaks.reserve(kBuckets);
    for (int b = 0; b < kBuckets; ++b) {
        const sf_count_t got = sf_readf_float(f, buf.data(), per);
        if (got <= 0) {
            break;
        }
        float peak = 0.0f;
        for (sf_count_t i = 0; i < got * info.channels; ++i) {
            const float a = std::fabs(buf[static_cast<size_t>(i)]);
            if (a > peak) {
                peak = a;
            }
        }
        m_peaks.append(peak);
    }
    sf_close(f);
    m_haveFile = !m_peaks.isEmpty();
    update();
}

void WaveformWidget::setPlayPos(double frac) {
    m_playPos = qBound(0.0, frac, 1.0);
    update();
}

void WaveformWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#0f1219"));
    const int n = m_peaks.size();
    if (n > 0) {
        const double bw = double(width()) / n;
        p.setPen(Qt::NoPen);
        p.setBrush(m_accent.darker(140));
        for (int i = 0; i < n; ++i) {
            const double h = m_peaks[i] * (height() - 8);
            p.drawRect(QRectF(i * bw, (height() - h) / 2.0, qMax(1.0, bw - 0.5), h));
        }
        // played region tint
        p.setBrush(QColor(255, 255, 255, 18));
        p.drawRect(QRectF(0, 0, m_playPos * width(), height()));
    } else {
        p.setPen(QColor("#5a6376"));
        p.drawText(rect(), Qt::AlignCenter, m_haveFile ? "" : "no waveform — load WAV / FLAC / OGG");
    }
    // beat-ish grid + playhead
    p.setPen(QColor(255, 255, 255, 26));
    for (int b = 1; b < 8; ++b) {
        const int x = width() * b / 8;
        p.drawLine(x, 0, x, height());
    }
    p.setPen(QColor("#e05252"));
    const int px = int(m_playPos * width());
    p.drawLine(px, 0, px, height());
}

void WaveformWidget::mousePressEvent(QMouseEvent* pEvent) {
    if (width() > 0) {
        emit seekClicked(qBound(0.0, double(pEvent->pos().x()) / width(), 1.0));
    }
}
