#include "BookmarksSource.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <print>

QVector<Entry> BookmarksSource::load() {
    QVector<Entry> out;
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/bookmarks.json";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return out;
    }

    QJsonParseError perr{};
    const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "bookmarks: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return out;
    }
    if (root.isEmpty()) {
        return out;
    }

    const QJsonArray arr = root.value("bookmarks").toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        Entry e;
        e.name = o.value("name").toString();
        e.exec = o.value("url").toString();
        if (!e.exec.startsWith("http://") && !e.exec.startsWith("https://")) {
            e.exec = "http://" + e.exec;
        }
        e.comment = o.value("comment").toString();
        e.icon = o.value("icon").toString();
        if (e.icon.isEmpty()) {
            e.icon = QStringLiteral("zen-browser");
        }
        e.lower_name = e.name.toLower();
        if (!e.name.isEmpty() && !e.exec.isEmpty()) {
            out.append(e);
        }
    }
    return out;
}
