#include "DesktopSource.h"
#include <QDir>
#include <QFile>
#include <QSet>
#include <QStandardPaths>

QVector<Entry> DesktopSource::load() {
    QVector<Entry> out;
    const QStringList dirs = QStandardPaths::standardLocations(
            QStandardPaths::ApplicationsLocation);

    QSet<QString> seen;
    seen.reserve(512);

    for (const auto& dir : dirs) {
        QDir d(dir);
        if (!d.exists()) { continue; }
        const auto files = d.entryList({ "*.desktop" }, QDir::Files);
        for (const auto& file : files) {
            if (seen.contains(file)) {
                continue;
            }
            seen.insert(file);
            Entry e;
            if (parse_desktop(d.filePath(file), e)) {
                out.append(e);
            }
        }
    }
    return out;
}

bool DesktopSource::parse_desktop(const QString& path, Entry& out) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { return false; }

    bool in_desktop_entry = false;
    bool is_application = false;
    bool no_display = false;
    bool hidden = false;
    QString name, exec, comment, icon;

    while (!f.atEnd()) {
        const QByteArray line = f.readLine();
        if (line.isEmpty()) { continue; }

        if (line.startsWith('[')) {
            in_desktop_entry = (line.trimmed() == "[Desktop Entry]");
            if (!in_desktop_entry && is_application) { break; }
            continue;
        }
        if (!in_desktop_entry) { continue; }

        int eq = static_cast<int>(line.indexOf('='));
        if (eq <= 0) { continue; }
        const QByteArray key = line.left(eq).trimmed();
        QByteArray value = line.mid(eq + 1);
        while (value.endsWith('\n') || value.endsWith('\r')) { value.chop(1); }

        if (key == "Type") {
            is_application = (value == "Application");
        }
        else if (key == "NoDisplay") {
            no_display = (value == "true");
        }
        else if (key == "Hidden") {
            hidden = (value == "true");
        }
        else if (key == "Name") {
            name = QString::fromUtf8(value);
        }
        else if (key == "Exec") {
            exec = QString::fromUtf8(value);
        }
        else if (key == "Comment") {
            comment = QString::fromUtf8(value);
        }
        else if (key == "Icon") {
            icon = QString::fromUtf8(value);
        }
    }

    if (!is_application || no_display || hidden || name.isEmpty()) { return false; }

    out.lower_name = name.toLower();
    out.name = std::move(name);
    out.exec = std::move(exec);
    out.comment = std::move(comment);
    out.icon = std::move(icon);
    return true;
}
