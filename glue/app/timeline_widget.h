// Timeline: paints the loaded arrangement (clips on a bar grid, xfade zone,
// playhead). Live mode has no engine mix clock, so the playhead runs on wall
// time since Play — honest approximation, good enough to follow the blend.
#pragma once

#include <QWidget>

#include "arrangement.h"

class TimelineWidget : public QWidget {
    Q_OBJECT

  public:
    explicit TimelineWidget(QWidget* pParent = nullptr);

    void setArrangement(const Arrangement& arrangement);
    void setPlayheadSec(double seconds); // -1 hides
    void clear();

  protected:
    void paintEvent(QPaintEvent*) override;

  private:
    Arrangement m_arr;
    bool m_hasArr = false;
    double m_playhead = -1.0;
};
