#include "EntryModel.h"
#include "BookmarksSource.h"
#include "DesktopSource.h"
#include "DotfilesSource.h"
#include "SshSource.h"
#include "ProjectsSource.h"
#include "ScriptsSource.h"
#include "BookmarkLauncher.h"
#include "DesktopLauncher.h"
#include "DotfileLauncher.h"
#include "SshLauncher.h"
#include "ProjectLauncher.h"
#include "ScriptLauncher.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <algorithm>
#include <QtContainerFwd>

static double fuzzy_score(const QString& term, const QString& target) {
    if (term.isEmpty()) {
        return 0.0;
    }
    if (target.isEmpty()) {
        return -1e9;
    }

    const int term_size = static_cast<int>(term.size());
    const int target_size = static_cast<int>(target.size());

    double score = 0.0;
    int term_index = 0;
    int last_match = -2;
    int consecutive = 0;

    for (int target_index = 0; target_index < target_size && term_index < term_size; ++target_index) {
        const QChar target_char = target.at(target_index);
        const QChar term_char = term.at(term_index);

        if (target_char != term_char) {
            continue;
        }

        double char_score = 0.0;

        if (target_index == 0) {
            char_score += 2.0;
        }
        else if ((target.at(target_index -1) == ' '
                    || target.at(target_index -1) == '-'
                    || target.at(target_index -1) == '_'
                    || target.at(target_index -1) == '.'
                    || target.at(target_index -1) == '/')) {
            char_score += 1.0;
        }

        if (last_match == target_index -1) {
            consecutive += 1;
            char_score += 1.0 + consecutive * 0.5;
        } else {
            consecutive = 0;
            if (last_match >= 0) {
                const int gap = target_index - last_match - 1;
                char_score -= gap * 0.5;
            }
        }

        score += char_score;
        last_match = target_index;
        ++term_index;
    }

    if (term_index < term_size) {
        return -1e9;
    }

    score -= static_cast<double>(target_size) * 0.01;

    return score;
}

static double fuzzy_score_multi(const QString& query, const QString& target) {
    const QStringList terms = query.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
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

static QString frecency_path() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
           + "/ahoiplank/frecency.json";
}

static_assert(static_cast<int>(EntryModel::Mode::Standard) == 0);
static_assert(static_cast<int>(EntryModel::Mode::Bookmarks) == 1);
static_assert(static_cast<int>(EntryModel::Mode::Dotfiles) == 2);
static_assert(static_cast<int>(EntryModel::Mode::SSH) == 3);
static_assert(static_cast<int>(EntryModel::Mode::Projects) == 4);
static_assert(static_cast<int>(EntryModel::Mode::Scripts) == 5);

EntryModel::EntryModel(QObject* parent) : QAbstractListModel(parent) {
    sources_[static_cast<size_t>(Mode::Standard)]  = std::make_unique<DesktopSource>();
    sources_[static_cast<size_t>(Mode::Bookmarks)] = std::make_unique<BookmarksSource>();
    sources_[static_cast<size_t>(Mode::Dotfiles)]  = std::make_unique<DotfilesSource>();
    sources_[static_cast<size_t>(Mode::SSH)]       = std::make_unique<SshSource>();
    sources_[static_cast<size_t>(Mode::Projects)]  = std::make_unique<ProjectsSource>();
    sources_[static_cast<size_t>(Mode::Scripts)]  = std::make_unique<ScriptsSource>();

    launchers_[static_cast<size_t>(Mode::Standard)]  = std::make_unique<DesktopLauncher>();
    launchers_[static_cast<size_t>(Mode::Bookmarks)] = std::make_unique<BookmarkLauncher>();
    launchers_[static_cast<size_t>(Mode::Dotfiles)]  = std::make_unique<DotfileLauncher>();
    launchers_[static_cast<size_t>(Mode::SSH)]       = std::make_unique<SshLauncher>();
    launchers_[static_cast<size_t>(Mode::Projects)]  = std::make_unique<ProjectLauncher>();
    launchers_[static_cast<size_t>(Mode::Scripts)]  = std::make_unique<ScriptLauncher>();

    load_frecency();

    flush_timer_ = new QTimer(this);
    flush_timer_->setSingleShot(true);
    flush_timer_->setInterval(30'000);
    QObject::connect(flush_timer_, &QTimer::timeout, this, &EntryModel::flush_frecency);

    rebuild();
}

EntryModel::~EntryModel() {
    flush_frecency();
}

int EntryModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(filtered_idx_.size());
}

QVariant EntryModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || idx.row() >= filtered_idx_.size() || idx.row() < 0) {
        return {};
    }
    const auto& e = all_.at(filtered_idx_.at(idx.row()));
    switch (role) {
        case Qt::UserRole + 1: return e.name;
        case Qt::UserRole + 2: return e.exec;
        case Qt::UserRole + 3: return e.comment;
        case Qt::UserRole + 4: return e.icon;
        default: break;
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

void EntryModel::set_mode(const Mode& mode) {
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
    rebuild();
    emit modeChanged();
}

void EntryModel::set_mode(const QString& mode_string) {
    Mode mode = mode_from_string(mode_string);
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
    rebuild();
    emit modeChanged();
}

void EntryModel::cycle_mode(int direction) {
    const int n = static_cast<int>(launchers_.size());
    int i = static_cast<int>(mode_);
    i = ((i + direction) % n + n) % n;
    set_mode(static_cast<Mode>(i));
}

QString EntryModel::mode_placeholder() const {
    switch (mode_) {
        case Mode::Bookmarks: return QStringLiteral("Search bookmarks…");
        case Mode::Dotfiles:  return QStringLiteral("Search dotfiles…");
        case Mode::SSH:       return QStringLiteral("Search or add SSH hosts…");
        case Mode::Projects:  return QStringLiteral("Search projects…");
        case Mode::Scripts:   return QStringLiteral("Search or run scripts…");
        case Mode::Standard:  return QStringLiteral("Search…");
        default: break;
    }
    return QStringLiteral("Search…");
}

void EntryModel::reload() {
    rebuild();
}

void EntryModel::request_reset() {
    emit resetRequested();
}

void EntryModel::rebuild() {
    current_filter_.clear();

    beginResetModel();
    all_ = sources_[static_cast<size_t>(mode_)]->load();

    std::sort(all_.begin(), all_.end(),
          [](const Entry& a, const Entry& b) {
              return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
          });

    filtered_idx_.clear();
    filtered_idx_.reserve(all_.size());
    for (int i = 0; i < all_.size(); ++i) {
        filtered_idx_.append(i);
    }

    std::sort(filtered_idx_.begin(), filtered_idx_.end(),
              [this](int a, int b) {
                  const Entry& ea = all_.at(a);
                  const Entry& eb = all_.at(b);
                  const double fa = frecency_score(ea);
                  const double fb = frecency_score(eb);
                  if (fa != fb) {
                      return fa > fb;
                  }
                  return QString::compare(ea.name, eb.name, Qt::CaseInsensitive) < 0;
              });

    prune_frecency();
    endResetModel();
}

void EntryModel::apply_filter_diff(const QVector<int>& new_idx) {
    QHash<int, int> current_pos;
    current_pos.reserve(filtered_idx_.size());
    for (int i = 0; i < filtered_idx_.size(); ++i) {
        current_pos.insert(filtered_idx_.at(i), i);
    }

    const QSet<int> new_set(new_idx.begin(), new_idx.end());

    for (int row = filtered_idx_.size() - 1; row >= 0; --row) {
        const int entry = filtered_idx_.at(row);
        if (new_set.contains(entry)) {
            continue;
        }

        beginRemoveRows(QModelIndex(), row, row);
        filtered_idx_.removeAt(row);
        current_pos.remove(entry);
        for (int i = row; i < filtered_idx_.size(); ++i) {
            current_pos[filtered_idx_.at(i)] = i;
        }
        endRemoveRows();
    }

    for (int target = 0; target < new_idx.size(); ++target) {
        const int desired = new_idx.at(target);
        const int current = current_pos.value(desired, -1);

        if (current < 0) {
            beginInsertRows(QModelIndex(), target, target);
            filtered_idx_.insert(target, desired);
            current_pos.insert(desired, target);
            for (int i = target + 1; i < filtered_idx_.size(); ++i) {
                current_pos[filtered_idx_.at(i)] = i;
            }
            endInsertRows();
            continue;
        }

        if (current == target) {
            continue;
        }
        const int destination = (current < target) ? target + 1 : target;

        beginMoveRows(QModelIndex(), current, current, QModelIndex(), destination);
        {
            const int entry = filtered_idx_.at(current);
            filtered_idx_.removeAt(current);
            const int insert_at = (destination > current) ? destination - 1 : destination;
            filtered_idx_.insert(insert_at, entry);

            for (int i = qMin(current, target); i <= qMax(current, target); ++i) {
                current_pos[filtered_idx_.at(i)] = i;
            }
        }
        endMoveRows();
    }
}

QString EntryModel::id_for(const Entry& e) const {
    return sources_[static_cast<size_t>(mode_)]->id_for(e);
}

void EntryModel::resort() {
    const QString q = current_filter_;
    const QString q_lower = q.toLower();

    QVector<int> new_idx;
    if (q.isEmpty()) {
        new_idx.reserve(all_.size());
        for (int i = 0; i < all_.size(); ++i) {
            new_idx.append(i);
        }
        std::sort(new_idx.begin(), new_idx.end(),
                  [this](int a, int b) {
                      const Entry& ea = all_.at(a);
                      const Entry& eb = all_.at(b);
                      const double fa = frecency_score(ea);
                      const double fb = frecency_score(eb);
                      if (fa != fb) {
                          return fa > fb;
                      }
                      return QString::compare(ea.name, eb.name, Qt::CaseInsensitive) < 0;
                  });
    }
    else {
        scored_buf_.clear();
        scored_buf_.reserve(all_.size());
        for (int i = 0; i < all_.size(); ++i) {
            const double s = fuzzy_score_multi(q_lower, all_.at(i).lower_name);
            if (s > -1e8) {
                scored_buf_.append({s, i});
            }
        }
        std::stable_sort(scored_buf_.begin(), scored_buf_.end(),
                         [this](const auto& a, const auto& b) {
                             if (a.score != b.score) {
                                 return a.score > b.score;
                             }

                             const Entry& ea = all_.at(a.idx);
                             const Entry& eb = all_.at(b.idx);
                             const double fa = frecency_score(ea);
                             const double fb = frecency_score(eb);
                             if (fa != fb) {
                                 return fa > fb;
                             }

                             return QString::compare(ea.name, eb.name, Qt::CaseInsensitive) < 0;
                         });
        new_idx.reserve(scored_buf_.size());
        for (const auto& p : scored_buf_) {
            new_idx.append(p.idx);
        }
    }

    apply_filter_diff(new_idx);
}

void EntryModel::set_filter(const QString& query) {
    const QString q = query.trimmed();
    if (q == current_filter_) {
        return;
    }
    current_filter_ = q;
    resort();
}

bool EntryModel::launch(int row, const QString& search_input) {
    bool user_entry = false;
    if (row < 0 || row >= filtered_idx_.size()) {
        if ((mode_ == Mode::SSH || mode_ == Mode::Scripts) && !search_input.isEmpty()) {
            user_entry = true;
        }
        else {
            if (mode_ != Mode::Standard && mode_ != Mode::Bookmarks) {
                const QString error_message = mode_to_string(mode_) +
                    " launch error: you are trying to run on "
                    + terminal_ + " terminal. Is it right?";
                emit errorOccurred(error_message);
                return false;
            }
            const QString error_message = mode_to_string(mode_) +
                " launch error: you are trying to run " + 
                search_input + ". Is it right?";
            emit errorOccurred(error_message);
            return false;
        }
    }
    if (user_entry) {
        Entry entry;
        entry.exec = search_input;
        const bool ok = launchers_[static_cast<size_t>(mode_)]
                            ->launch(entry, text_editor_, terminal_);
        if (!ok) {
            if (mode_ != Mode::Standard && mode_ != Mode::Bookmarks) {
                const QString error_message = mode_to_string(mode_) +
                    " launch error: you are trying to run " +
                    search_input + " on " + terminal_ + ". Is it right?";
                emit errorOccurred(error_message);
                return ok;
            }
            const QString error_message = mode_to_string(mode_) +
                " launch error: you are trying to run " + 
                entry.exec + ". Is it right?";
            emit errorOccurred(error_message);
            return ok;
        }
        bump_frecency(entry);
        resort();
        emit dismissed();
        return ok;
    }
    const Entry& entry = all_.at(filtered_idx_.at(row));
    const bool ok = launchers_[static_cast<size_t>(mode_)]
                        ->launch(entry, text_editor_, terminal_);
    if (!ok) {
        if (mode_ != Mode::Standard && mode_ != Mode::Bookmarks) {
            const QString error_message = mode_to_string(mode_) +
                " launch error: you are trying to run " +
                entry.exec + " on " +
                terminal_ + ". Is it right?";
            emit errorOccurred(error_message);
            return ok;
        }
        const QString error_message = mode_to_string(mode_) +
            " launch error: you are trying to run " + 
            entry.exec + ". Is it right?";
        emit errorOccurred(error_message);
        return ok;
    }
    bump_frecency(entry);
    resort();
    emit dismissed();
    return ok;
}

void EntryModel::quit() {
    QCoreApplication::quit();
}

void EntryModel::dismiss() {
    emit dismissed();
}

void EntryModel::load_frecency() {
    QFile f(frecency_path());
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    if (root.isEmpty()) {
        return;
    }

    const int version = root.value(QStringLiteral("version")).toInt();
    if (version != 1) {
        qWarning() << "frecency: unknown version" << version << "- starting fresh";
        return;
    }

    const QJsonObject modes = root.value(QStringLiteral("modes")).toObject();
    for (auto mode_it = modes.begin(); mode_it != modes.end(); ++mode_it) {
        const QJsonObject entries = mode_it.value().toObject();
        QHash<QString, FrecencyRecord>& bucket = frecency_[mode_it.key()];

        for (auto entry_it = entries.begin(); entry_it != entries.end(); ++entry_it) {
            const QJsonObject rec = entry_it.value().toObject();
            FrecencyRecord r;
            r.count     = rec.value(QStringLiteral("count")).toInt();
            r.last_used = static_cast<qint64>(rec.value(QStringLiteral("last")).toDouble());
            if (r.count > 0) {
                bucket.insert(entry_it.key(), r);
            }
        }
    }
}

void EntryModel::flush_frecency() {
    QJsonObject modes_obj;
    for (auto mode_it = frecency_.constBegin(); mode_it != frecency_.constEnd(); ++mode_it) {
        QJsonObject entries_obj;
        for (auto entry_it = mode_it.value().constBegin(); entry_it != mode_it.value().constEnd(); ++entry_it) {
            QJsonObject rec;
            rec[QStringLiteral("count")] = entry_it.value().count;
            rec[QStringLiteral("last")]  = static_cast<double>(entry_it.value().last_used);
            entries_obj[entry_it.key()] = rec;
        }
        modes_obj[mode_it.key()] = entries_obj;
    }

    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("modes")]   = modes_obj;

    const QString path = frecency_path();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "frecency: could not open" << path << "for writing";
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (!f.commit()) {
        qWarning() << "frecency: commit failed for" << path;
    }
}

void EntryModel::schedule_flush() {
    if (flush_timer_ && !flush_timer_->isActive()) {
        flush_timer_->start();
    }
}

void EntryModel::bump_frecency(const Entry& e) {
    const QString id = id_for(e);
    if (id.isEmpty()) {
        return;
    }

    FrecencyRecord& r = frecency_[mode_string()][id];
    r.count     = qMin(200, r.count + 1);
    r.last_used = QDateTime::currentSecsSinceEpoch();

    schedule_flush();
}

double EntryModel::frecency_score(const Entry& e) const {
    auto mode_it = frecency_.constFind(mode_string());
    if (mode_it == frecency_.constEnd()) {
        return 0.0;
    }
    auto rec_it = mode_it.value().constFind(id_for(e));
    if (rec_it == mode_it.value().constEnd()) {
        return 0.0;
    }

    const FrecencyRecord& r = rec_it.value();
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 age = now - r.last_used;

    double weight;
    if (age <   6 * 3600) { weight = 100.0; }
    else if (age <  24 * 3600) { weight = 80.0; }
    else if (age <   7 * 86400) { weight = 60.0; }
    else if (age <  30 * 86400) { weight = 40.0; }
    else if (age <  90 * 86400) { weight = 20.0; }
    else { weight = 10.0; }

    return r.count * weight;
}

void EntryModel::prune_frecency() {
    if (all_.isEmpty()) {
        return;
    }

    auto mode_it = frecency_.find(mode_string());
    if (mode_it == frecency_.end()) {
        return;
    }

    QSet<QString> live_ids;
    live_ids.reserve(all_.size());
    for (const Entry& e : all_) {
        live_ids.insert(id_for(e));
    }

    auto& bucket = mode_it.value();
    for (auto it = bucket.begin(); it != bucket.end(); ) {
        if (!live_ids.contains(it.key())) {
            it = bucket.erase(it);
        }
        else {
            ++it;
        }
    }
}
