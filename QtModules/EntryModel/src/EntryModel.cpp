#include "EntryModel.h"
#include "BookmarksSource.h"
#include "DesktopSource.h"
#include "DotfilesSource.h"
#include "SshSource.h"
#include "BookmarkLauncher.h"
#include "DesktopLauncher.h"
#include "DotfileLauncher.h"
#include "SshLauncher.h"
#include <QCoreApplication>
#include <algorithm>

namespace {

double fuzzy_score(const QString& term, const QString& target) {
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

        if (target_char.toLower() != term_char.toLower()) {
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
                    || target.at(target_index -1) == '/')
                ||(target.at(target_index -1).isLower() && target_char.isUpper())) {
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

static_assert(static_cast<size_t>(EntryModel::Mode::SSH) == 3,
              "keep array sizes in sync with Mode enum");

EntryModel::EntryModel(QObject* parent) : QAbstractListModel(parent) {
    sources_[static_cast<size_t>(Mode::Standard)]  = std::make_unique<DesktopSource>();
    sources_[static_cast<size_t>(Mode::Bookmarks)] = std::make_unique<BookmarksSource>();
    sources_[static_cast<size_t>(Mode::Dotfiles)]  = std::make_unique<DotfilesSource>();
    sources_[static_cast<size_t>(Mode::SSH)]       = std::make_unique<SshSource>();

    launchers_[static_cast<size_t>(Mode::Standard)]  = std::make_unique<DesktopLauncher>();
    launchers_[static_cast<size_t>(Mode::Bookmarks)] = std::make_unique<BookmarkLauncher>();
    launchers_[static_cast<size_t>(Mode::Dotfiles)]  = std::make_unique<DotfileLauncher>();
    launchers_[static_cast<size_t>(Mode::SSH)]       = std::make_unique<SshLauncher>();

    rebuild();
}

int EntryModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(filtered_idx_.size());
}

QVariant EntryModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || idx.row() >= filtered_idx_.size()) {
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
    endResetModel();
}

void EntryModel::set_filter(const QString& query) {
    const QString q = query.trimmed();

    if (q == current_filter_) {
        return;
    }
    current_filter_ = q;

    beginResetModel();

    if (q.isEmpty()) {
        filtered_idx_.clear();
        filtered_idx_.reserve(all_.size());
        for (int i = 0; i < all_.size(); ++i) {
            filtered_idx_.append(i);
        }
        endResetModel();
        return;
    }

    scored_buf_.clear();
    scored_buf_.reserve(all_.size());

    for (int i = 0; i < all_.size(); ++i) {
        const double s = fuzzy_score_multi(q, all_.at(i).lower_name);
        if (s > -1e8) {
            scored_buf_.append({ s, i });
        }
    }

    std::sort(scored_buf_.begin(), scored_buf_.end(),
              [](const QPair<double, int>& a, const QPair<double, int>& b) {
                  return a.first > b.first;
              });

    filtered_idx_.clear();
    filtered_idx_.reserve(scored_buf_.size());
    for (const auto& pair : scored_buf_) {
        filtered_idx_.append(pair.second);
    }

    endResetModel();
}

bool EntryModel::launch(int row) {
    if (row < 0 || row >= filtered_idx_.size()) {
        return false;
    }
    const Entry& entry = all_.at(filtered_idx_.at(row));
    const bool ok = launchers_[static_cast<size_t>(mode_)]
                        ->launch(entry, text_editor_, terminal_);
    if (ok) {
        emit dismissed();
    }
    return ok;
}

void EntryModel::quit() {
    QCoreApplication::quit();
}

void EntryModel::dismiss() {
    emit dismissed();
}
