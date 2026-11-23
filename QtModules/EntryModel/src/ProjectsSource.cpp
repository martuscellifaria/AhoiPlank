#include "ProjectsSource.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <print>

QVector<Entry> ProjectsSource::load() {
    QVector<Entry> out;
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/projects.json";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return out;
    }

    QJsonParseError perr{};
    const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "projects: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return out;
    }
    if (root.isEmpty()) {
        return out;
    }

    const QJsonArray arr = root.value("projects").toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        Entry e;
        e.name = o.value("name").toString();
        e.exec = o.value("path").toString();
        e.id = e.exec;
        e.comment = o.value("path").toString();
        e.icon = o.value("icon").toString();
        if (e.icon.isEmpty()) {
            e.icon = QStringLiteral("gvim");
        }
        e.lower_name = e.name.toLower();
        if (!e.name.isEmpty() && !e.exec.isEmpty()) {
            out.append(e);
        }
    }
    return out;
}

void ProjectsSource::pin_project() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/projects.json";
    const QDir cwd = QDir::current();
    const QString project_path = cwd.absolutePath();
    const QString project_name = cwd.dirName();

    QJsonObject root;
    QJsonArray arr;

    if (QFileInfo::exists(path)) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            std::println(stderr, "projects: failed to open {} for reading ({})",
                         path.toStdString(), f.errorString().toStdString());
            return;
        }

        const QByteArray raw = f.readAll();
        f.close();

        if (!raw.isEmpty()) {
            QJsonParseError perr{};
            root = QJsonDocument::fromJson(raw, &perr).object();
            if (perr.error != QJsonParseError::NoError) {
                std::println(stderr, "projects: {} is not valid json ({})",
                             path.toStdString(), perr.errorString().toStdString());
                return;
            }
            arr = root.value("projects").toArray();
        }
    }
    else {
        QDir().mkpath(QFileInfo(path).absolutePath());
    }

    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        if (o.value("path").toString() == project_path &&
            o.value("name").toString() == project_name) {
            return;
        }
    }

    QJsonObject entry;
    entry.insert("name", project_name);
    entry.insert("path", project_path);
    entry.insert("icon", "gvim");
    arr.append(entry);
    root.insert("projects", arr);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::println(stderr, "projects: failed to open {} for writing ({})",
                     path.toStdString(), f.errorString().toStdString());
        return;
    }

    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void ProjectsSource::unpin_project(const QString& project_name) {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/projects.json";
    if (!QFileInfo::exists(path)) {
        std::println(stderr, "Cannot find a projects.json file inside ~/.config/ahoiplank/. Exiting.");
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        std::println(stderr, "projects: failed to open {} for reading ({})",
                     path.toStdString(), f.errorString().toStdString());
        return;
    }

    QJsonParseError perr{};
    QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    f.close();

    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "projects: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return;
    }

    QJsonArray updated;
    bool removed = false;
    const QDir cwd = QDir::current();
    QString name = project_name;
    const QJsonArray arr = root.value("projects").toArray();
    if (name.isEmpty()) {
        name = cwd.dirName();
        const QString project_path = cwd.absolutePath();
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            if (o.value("path").toString() == project_path &&
                o.value("name").toString() == name) {
                removed = true;
                continue;
            }
            updated.append(v);
        }
    }
    else {
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            if (o.value("name").toString() == name) {
                removed = true;
                continue;
            }
            updated.append(v);
        }
    }
    if (!removed) {
        std::println(stderr, "Project {} is not pinned. Exiting.",
                     name.toStdString());
        return;
    }

    root.insert("projects", updated);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::println(stderr, "projects: failed to open {} for writing ({})",
                     path.toStdString(), f.errorString().toStdString());
        return;
    }

    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void ProjectsSource::list_projects() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/projects.json";
    if (!QFileInfo::exists(path)) {
        std::println(stderr, "Cannot find a projects.json file inside ~/.config/ahoiplank/. Exiting.");
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        std::println(stderr, "projects: failed to open {} for reading ({})",
                     path.toStdString(), f.errorString().toStdString());
        return;
    }

    QJsonParseError perr{};
    QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    f.close();

    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "projects: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return;
    }
    std::string list = "NAME\t\tPATH";
    if (!root.isEmpty() && root.contains("projects")) {
        const QJsonArray arr = root.value("projects").toArray();

        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            list = std::format("{}\n{}\t{}",
                    list,
                    o.value("name").toString().toStdString(),
                    o.value("path").toString().toStdString());
        }
    }
    std::println("{}", list);
}
