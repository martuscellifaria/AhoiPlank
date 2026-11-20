#include "ScriptsSource.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <print>

QVector<Entry> ScriptsSource::load() {
    QVector<Entry> out;
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/scripts.json";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return out;
    }

    QJsonParseError perr{};
    const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "scripts: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return out;
    }
    if (root.isEmpty()) {
        return out;
    }

    const QJsonArray arr = root.value("scripts").toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        Entry e;
        e.name = o.value("name").toString();
        e.exec = o.value("script").toString();
        e.id = e.exec;
        e.comment = o.value("script").toString();
        e.persistent_terminal = o.value("persistent_terminal").toBool();
        e.icon = o.value("icon").toString();
        if (e.icon.isEmpty()) {
            e.icon = QStringLiteral("terminal");
        }
        e.lower_name = e.name.toLower();
        if (!e.name.isEmpty() && !e.exec.isEmpty()) {
            out.append(e);
        }
    }
    return out;
}
