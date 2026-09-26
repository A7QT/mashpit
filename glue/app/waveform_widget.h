// Waveform: peaks read straight from the loaded file (libsndfile covers
// WAV/FLAC/OGG; anything else shows a placeholder). Click seeks.
#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

class WaveformWidget : public QWidget {
    Q_OBJECT

  public:
    explicit WaveformWidget(const QColor& accent, QWidget* pParent = nullptr);

    void setFile(const QString& path); // empty path clears
    void setPlayPos(double frac);

  signals:
    void seekClicked(double frac);

  protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* pEvent) override;

  private:
    QColor m_accent;
    QVector<float> m_peaks;
    double m_playPos = 0.0;
    bool m_haveFile = false;
};
