#include "ScriptLauncher.h"
#include "EntryModel.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <exception>
#include <print>
#include <qstringview.h>
#include <stdexcept>

bool ScriptLauncher::launch(const Entry& entry,
                         const QString&,
                         const QString& terminal_cfg) {
    const QString terminal = TerminalLauncher::find_terminal(terminal_cfg);
    if (terminal.isEmpty()) {
        std::println(stderr, "script: no terminal found, set terminal in config.json");
        return false;
    }
    try {
        const auto stored_script = store_script(entry);
        if (entry.persistent_terminal || stored_script.first) {
            const QString wrapped_cmd = QStringLiteral("%1; exec \"${SHELL:-sh}\"").arg(stored_script.second);
            return TerminalLauncher::run_in_terminal_argv(terminal, { "sh", "-c", wrapped_cmd });
        }
        return TerminalLauncher::run_in_terminal_argv(terminal, { "sh", "-c", stored_script.second });
    }
    catch (const std::exception& err) {
        std::println(stderr, "{}", err.what());
        return false;
    }
}

std::pair<bool, QString> ScriptLauncher::store_script(const Entry& entry) {
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
            throw std::runtime_error(std::format("scripts: failed to open {} for reading ({})",
                         path.toStdString(), f.errorString().toStdString()));
        }

        const QByteArray raw = f.readAll();
        f.close();

        if (!raw.isEmpty()) {
            QJsonParseError perr{};
            root = QJsonDocument::fromJson(raw, &perr).object();
            if (perr.error != QJsonParseError::NoError) {
                throw std::runtime_error(std::format("scripts: failed to open {} for reading ({})",
                             path.toStdString(), f.errorString().toStdString()));
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
            return { false, entry.exec };
        }
    }

    bool persistent_terminal = false;
    QString persistent = "--persistent ";
    QString exec_command = entry.exec;
    if (exec_command.startsWith(persistent)) {
        persistent_terminal = true;
        exec_command = exec_command.sliced(persistent.length());
    }


    QJsonObject entry_obj;
    entry_obj.insert("name", exec_command);
    entry_obj.insert("script", exec_command);
    entry_obj.insert("icon", "utilities-terminal");
    entry_obj.insert("persistent_terminal", persistent_terminal);
    arr.append(entry_obj);
    root.insert("scripts", arr);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw std::runtime_error(std::format("scripts: failed to open {} for reading ({})",
                     path.toStdString(), f.errorString().toStdString()));
    }

    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return { persistent_terminal, exec_command };
}

