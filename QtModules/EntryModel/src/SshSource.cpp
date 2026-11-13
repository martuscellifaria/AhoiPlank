#include "SshSource.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <print>

QVector<Entry> SshSource::load() {
    QVector<Entry> out;
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/ssh.json";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return out;
    }

    QJsonParseError perr{};
    const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "ssh: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return out;
    }
    if (root.isEmpty()) {
        return out;
    }

    const QJsonArray arr = root.value("ssh").toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        Entry e;
        e.name = o.value("name").toString();
        e.exec = o.value("user").toString() + "@" + o.value("host").toString();
        e.icon = o.value("icon").toString();
        if (e.icon.isEmpty()) {
            e.icon = QStringLiteral("network-server");
        }
        e.lower_name = e.name.toLower();
        if (!e.name.isEmpty() && !e.exec.isEmpty()) {
            out.append(e);
        }
    }
    return out;
}
