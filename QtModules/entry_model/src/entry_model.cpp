#include "entry_model.h"
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QIcon>
#include <QPixmap>
#include <algorithm>

entry_model::entry_model(QObject* parent) : QAbstractListModel(parent) {
    load_desktop_files();
}

int entry_model::rowCount(const QModelIndex&) const {
    return filtered_.size();
}

QVariant entry_model::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || idx.row() >= filtered_.size()) {
        return {};
    }
    const auto& e = filtered_.at(idx.row());
    switch (role) {
        case Qt::UserRole + 1: return e.name;
        case Qt::UserRole + 2: return e.exec;
        case Qt::UserRole + 3: return e.comment;
        case Qt::UserRole + 4: return e.icon;
    }
    return {};
}

QHash<int, QByteArray> entry_model::roleNames() const {
    return {
        { Qt::UserRole + 1, "name"    },
        { Qt::UserRole + 2, "exec"    },
        { Qt::UserRole + 3, "comment" },
        { Qt::UserRole + 4, "icon"    },
    };
}

void entry_model::set_filter(const QString& query) {
    beginResetModel();
    filtered_.clear();
    const QString q = query.trimmed().toLower();
    for (const auto& e : all_) {
        if (q.isEmpty() || e.name.toLower().contains(q)) {
            filtered_.append(e);
        }
    }
    endResetModel();
}

bool entry_model::launch(int row) {
    if (row < 0 || row >= filtered_.size()) {
        return false;
    }
    const auto& e = filtered_.at(row);
    QString exec = e.exec;
    static const QRegularExpression field_code(R"(%[fFuUdDnNickvm])");
    exec.remove(field_code);
    QStringList argv = QProcess::splitCommand(exec);
    if (argv.isEmpty()) {
        return false;
    }
    const bool ok = QProcess::startDetached(argv.takeFirst(), argv);
    if (ok) {
        QCoreApplication::quit();
    }
    return ok;
}

void entry_model::quit() {
    QCoreApplication::quit();
}

void entry_model::load_desktop_files() {
    const QStringList dirs = {
        QDir::homePath() + "/.local/share/applications",
        "/usr/local/share/applications",
        "/usr/share/applications",
    };

    for (const auto& dir : dirs) {
        QDir d(dir);
        if (!d.exists()) {
            continue;
        }

        const auto files = d.entryList({ "*.desktop" }, QDir::Files);
        for (const auto& file : files) {
            QSettings s(d.filePath(file), QSettings::IniFormat);
            s.beginGroup("Desktop Entry");

            if (s.value("Type").toString() != "Application"
                || s.value("NoDisplay", false).toBool()
                || s.value("Hidden", false).toBool()) {
                s.endGroup();
                continue;
            }

            all_.append({
                s.value("Name").toString(),
                s.value("Exec").toString(),
                s.value("Comment").toString(),
                s.value("Icon").toString(),
            });
            s.endGroup();
        }
    }

    std::sort(all_.begin(), all_.end(),
              [](const entry& a, const entry& b) { return a.name < b.name; });

    set_filter("");
}


icon_provider::icon_provider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

QPixmap icon_provider::requestPixmap(const QString& id, QSize* size,
                                     const QSize& requested) {
    const QSize target = requested.isValid() ? requested : QSize(32, 32);

    QIcon icon = QIcon::fromTheme(id);

    if (icon.isNull()) {
        icon = QIcon::fromTheme("application-x-executable");
    }

    QPixmap pm = icon.pixmap(target);
    if (size) {
        *size = pm.size();
    }
    return pm;
}
