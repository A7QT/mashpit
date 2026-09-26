// mashpit app v1 — arrangement model (subset of mashup.json v1, Qt side).
//-Clips place files on a bar grid; an optional mix entry marks the blend.
// Files resolve relative to the JSON location (same rule as the CLI).
#pragma once

#include <QString>
#include <QVector>

struct ArrangementClip {
    QString track;
    QString file;
    double atBar = 1.0;
    double bars = 8.0;
    double tempoRatio = 1.0;
    double pitchSemi = 0.0;
    QString role;
};

struct ArrangementMix {
    double atBar = 1.0;
    double bars = 8.0;
    bool present = false;
};

struct Arrangement {
    double bpm = 128.0;
    QVector<ArrangementClip> clips;
    ArrangementMix mix;

    double secPerBar() const {
        return 60.0 / bpm * 4.0;
    }
    double clipStartSec(const ArrangementClip& clip) const {
        return (clip.atBar - 1.0) * secPerBar();
    }
    double totalSec() const {
        double end = 0.0;
        for (const auto& c : clips) {
            end = qMax(end, clipStartSec(c) + c.bars * secPerBar());
        }
        return end + 2.0;
    }

    // Loads a v1 mashup.json. Returns error text, empty on success.
    // Only the validated subset is read (validate with `mashpit validate`).
    static Arrangement load(const QString& path, QString* pError);
};
