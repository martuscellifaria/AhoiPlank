#include "ScriptLauncher.h"
#include "EntryModel.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <print>

bool ScriptLauncher::launch(const Entry& entry,
                         const QString&,
                         const QString& terminal_cfg) {
    const QString terminal = TerminalLauncher::find_terminal(terminal_cfg);
    if (terminal.isEmpty()) {
        std::println(stderr, "script: no terminal found, set terminal in config.json");
        return false;
    }
    bool new_entry = store_script(entry);
    if (entry.persistent_terminal || new_entry) {
        const QString wrapped_cmd = QStringLiteral("%1; exec \"${SHELL:-sh}\"").arg(entry.exec);
        return TerminalLauncher::run_in_terminal_argv(terminal, { "sh", "-c", wrapped_cmd });
    }
    return TerminalLauncher::run_in_terminal_argv(terminal, { "sh", "-c", entry.exec });
}

bool ScriptLauncher::store_script(const Entry& entry) {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/scripts.json";
    const QDir cwd = QDir::current();
    const QString project_path = cwd.absolutePath();
    const QString project_name = cwd.dirName();

    QJsonObject root;
    QJsonArray arr;

    if (QFileInfo::exists(path)) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            std::println(stderr, "scripts: failed to open {} for reading ({})",
                         path.toStdString(), f.errorString().toStdString());
            return true;
        }

        const QByteArray raw = f.readAll();
        f.close();

        if (!raw.isEmpty()) {
            QJsonParseError perr{};
            root = QJsonDocument::fromJson(raw, &perr).object();
            if (perr.error != QJsonParseError::NoError) {
                std::println(stderr, "scripts: {} is not valid json ({})",
                             path.toStdString(), perr.errorString().toStdString());
                return true;
            }
            arr = root.value("scripts").toArray();
        }
    }
    else {
        QDir().mkpath(QFileInfo(path).absolutePath());
    }

    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        if (o.value("script").toString() == entry.exec) {
            return false;
        }
    }

    QJsonObject entry_obj;
    entry_obj.insert("name", entry.exec);
    entry_obj.insert("script", entry.exec);
    entry_obj.insert("icon", "utilities-terminal");
    entry_obj.insert("persistent_terminal", true);
    arr.append(entry_obj);
    root.insert("scripts", arr);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::println(stderr, "projects: failed to open {} for writing ({})",
                     path.toStdString(), f.errorString().toStdString());
        return true;
    }

    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

