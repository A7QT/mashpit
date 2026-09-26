#include "arrangement.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Arrangement Arrangement::load(const QString& path, QString* pError) {
    Arrangement out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        *pError = QString("cannot open %1").arg(path);
        return out;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        *pError = QString("bad JSON: %1").arg(parseError.errorString());
        return out;
    }
    const QJsonObject root = doc.object();
    out.bpm = root.value("projectBpm").toDouble(128.0);
    QHash<QString, QPair<QString, QString>> trackFiles; // id -> (file, role)
    const QDir base(QFileInfo(path).dir());
    for (const auto& t : root.value("tracks").toArray()) {
        const QJsonObject o = t.toObject();
        QString file = o.value("file").toString();
        if (!file.isEmpty() && QFileInfo(file).isRelative()) {
            file = base.filePath(file);
        }
        trackFiles[o.value("id").toString()] =
                qMakePair(file, o.value("role").toString());
    }
    for (const auto& c : root.value("clips").toArray()) {
        const QJsonObject o = c.toObject();
        ArrangementClip clip;
        clip.track = o.value("track").toString();
        clip.atBar = o.value("atBar").toDouble(1.0);
        clip.bars = o.value("bars").toDouble(8.0);
        clip.tempoRatio = o.value("tempoRatio").toDouble(1.0);
        clip.pitchSemi = o.value("pitchSemi").toDouble(0.0);
        if (trackFiles.contains(clip.track)) {
            clip.file = trackFiles[clip.track].first;
            clip.role = trackFiles[clip.track].second;
        }
        out.clips.append(clip);
    }
    const QJsonArray mix = root.value("mix").toArray();
    if (!mix.isEmpty()) {
        const QJsonObject o = mix.first().toObject();
        out.mix.present = true;
        out.mix.atBar = o.value("atBar").toDouble(1.0);
        out.mix.bars = o.value("bars").toDouble(8.0);
    }
    return out;
}
