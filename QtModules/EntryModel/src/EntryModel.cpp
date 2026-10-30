#include "EntryModel.h"
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QIcon>
#include <QPixmap>
#include <algorithm>

namespace {

double fuzzy_score(const QString& q, const QString& s) {
    if (q.isEmpty()) {
        return 0.0;
    }
    if (s.isEmpty()) {
        return -1e9;
    }

    const int qn = q.size();
    const int sn = s.size();

    double score = 0.0;
    int qi = 0;
    int last_match = -2;
    int consecutive = 0;

    for (int si = 0; si < sn && qi < qn; ++si) {
        const QChar sc = s.at(si);
        const QChar qc = q.at(qi);

        if (sc.toLower() != qc.toLower()) {
            continue;
        }

        double char_score = 0.0;

        if (si == 0) {
            char_score += 2.0;
        } else if (s.at(si - 1) == ' ' || s.at(si - 1) == '-'
                   || s.at(si - 1) == '_' || s.at(si - 1) == '.'
                   || s.at(si - 1) == '/') {
            char_score += 1.0;
        } else if (s.at(si - 1).isLower() && sc.isUpper()) {
            char_score += 1.0;
        }

        if (last_match == si - 1) {
            consecutive += 1;
            char_score += 1.0 + consecutive * 0.5;
        } else {
            consecutive = 0;
            if (last_match >= 0) {
                const int gap = si - last_match - 1;
                char_score -= gap * 0.5;
            }
        }

        score += char_score;
        last_match = si;
        ++qi;
    }

    if (qi < qn) {
        return -1e9;
    }

    score -= static_cast<double>(sn) * 0.01;

    return score;
}

double fuzzy_score_multi(const QString& query, const QString& target) {
    const QStringList terms = query.split(' ', Qt::SkipEmptyParts);
    if (terms.isEmpty()) {
        return 0.0;
    }

    double total = 0.0;
    for (const auto& term : terms) {
        const double s = fuzzy_score(term, target);
        if (s < -1e8) {
            return -1e9;
        }
        total += s;
    }
    return total;
}

} // namespace

EntryModel::EntryModel(QObject* parent) : QAbstractListModel(parent) {
    load_desktop_files();
}

int EntryModel::rowCount(const QModelIndex&) const {
    return filtered_.size();
}

QVariant EntryModel::data(const QModelIndex& idx, int role) const {
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

QHash<int, QByteArray> EntryModel::roleNames() const {
    return {
        { Qt::UserRole + 1, "name"    },
        { Qt::UserRole + 2, "exec"    },
        { Qt::UserRole + 3, "comment" },
        { Qt::UserRole + 4, "icon"    },
    };
}

void EntryModel::set_filter(const QString& query) {
    beginResetModel();

    const QString q = query.trimmed();

    if (q.isEmpty()) {
        filtered_ = all_;
        endResetModel();
        return;
    }

    QVector<QPair<double, int>> scored;
    scored.reserve(all_.size());

    for (int i = 0; i < all_.size(); ++i) {
        const double s = fuzzy_score_multi(q, all_.at(i).lower_name);
        if (s > -1e8) {
            scored.append({ s, i });
        }
    }

    std::sort(scored.begin(), scored.end(),
              [](const QPair<double, int>& a, const QPair<double, int>& b) {
                  return a.first > b.first;
              });

    filtered_.clear();
    filtered_.reserve(scored.size());
    for (const auto& pair : scored) {
        filtered_.append(all_.at(pair.second));
    }

    endResetModel();
}

bool EntryModel::launch(int row) {
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

void EntryModel::quit() {
    QCoreApplication::quit();
}

void EntryModel::load_desktop_files() {
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

	    entry e;
            e.name = s.value("Name").toString();
            e.exec = s.value("Exec").toString();
            e.comment = s.value("Comment").toString();
            e.icon = s.value("Icon").toString();
            e.lower_name = e.name.toLower();
            all_.append(e);
            s.endGroup();
        }
    }

    std::sort(all_.begin(), all_.end(),
              [](const entry& a, const entry& b) { return a.name < b.name; });

    set_filter("");
}


IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

QPixmap IconProvider::requestPixmap(const QString& id, QSize* size,
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
