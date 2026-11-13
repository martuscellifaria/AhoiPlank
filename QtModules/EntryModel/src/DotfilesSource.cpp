#include "DotfilesSource.h"
#include <QDir>
#include <QStandardPaths>

QVector<Entry> DotfilesSource::load() {
    QVector<Entry> out;
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);

    QDir config_dir(path);
    if (!config_dir.exists()) {
        return out;
    }

    const auto dirs = config_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& name : dirs) {
        if (name.startsWith(".")) {
            continue;
        }
        Entry e;
        e.name = name;
        e.exec = config_dir.filePath(name);
        e.comment = e.exec;
        e.icon = QStringLiteral("folder");
        e.lower_name = name.toLower();
        out.append(e);
    }
    return out;
}
