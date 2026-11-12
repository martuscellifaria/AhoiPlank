#include "EntryModel.h"
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSet>
#include <algorithm>
#include <print>
#include <qstandardpaths.h>

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

EntryModel::EntryModel(QObject* parent) : QAbstractListModel(parent) {
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

void EntryModel::set_mode(const QString& mode) {
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
    all_.clear();
    filtered_idx_.clear();

    if (mode_ == "bookmark") {
        load_bookmarks();
    }
    else if (mode_ == "dotfiles") {
        load_dotfiles();
    }
    else {
        load_desktop_files();
    }

    std::sort(all_.begin(), all_.end(),
          [](const Entry& a, const Entry& b) {
              return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
          });

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
    const auto& e = all_.at(filtered_idx_.at(row));

    if (mode_ == "bookmark") {
        const bool ok = QProcess::startDetached("xdg-open", { e.exec });
        if (ok) {
            emit dismissed();
        }
        return ok;
    }

    if (mode_ == "dotfiles") {
        QString terminal = text_editor_terminal_;
        if (terminal.isEmpty()) {
            terminal = qEnvironmentVariable("TERMINAL");
        }
        if (terminal.isEmpty()) {
            static const char* const candidates[] = {
                "alacritty", "foot", "kitty", "wezterm", "ghostty",
                "konsole", "gnome-terminal", "xfce4-terminal", "xterm",
                nullptr,
            };
            for (const char* const* c = candidates; *c; ++c) {
                if (!QStandardPaths::findExecutable(QString::fromLatin1(*c)).isEmpty()) {
                    terminal = QString::fromLatin1(*c);
                    break;
                }
            }
        }
        if (terminal.isEmpty()) {
            std::println(stderr, "dotfiles: no terminal found, set text_editor_terminal in config.json");
            return false;
        }
        QString quoted_path = e.exec;
        quoted_path.replace('\'', QStringLiteral("'\\''"));
        const QString script = QStringLiteral("cd '%1' && exec %2 .").arg(quoted_path, text_editor_);

        QStringList args;
        if (terminal.endsWith("gnome-terminal")) {
            args = { "--", "sh", "-c", script };
        }
        else if (terminal.endsWith("wezterm")) {
            args = { "start", "--", "sh", "-c", script };
        }
        else if (terminal.endsWith("xfce4-terminal")) {
            args = { "-x", "sh", "-c", script };
        }
        else {
            args = { "-e", "sh", "-c", script };
        }

        const bool ok = QProcess::startDetached(terminal, args);
        if (ok) {
            emit dismissed();
        }
        return ok;
    }

    QString exec = e.exec;
    {
        QString out;
        out.reserve(exec.size());
        static const QString codes = QStringLiteral("fFuUdDnNickvm");
        for (int i = 0; i < exec.size(); ++i) {
            if (exec.at(i) == u'%' && i + 1 < exec.size()) {
                const QChar c = exec.at(i + 1);
                if (c == u'%') { out += u'%'; ++i; continue; }
                if (codes.contains(c)) { ++i; continue; }
            }
            out += exec.at(i);
        }
        exec = std::move(out);
    }
    QStringList argv = QProcess::splitCommand(exec);
    if (argv.isEmpty()) {
        return false;
    }
    const bool ok = QProcess::startDetached(argv.takeFirst(), argv);
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

void EntryModel::load_desktop_files() {
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
                all_.append(e);
            }
        }
    }
}

void EntryModel::load_bookmarks() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/bookmarks.json";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    if (root.isEmpty()) {
        return;
    }

    const QJsonArray arr = root.value("bookmarks").toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        Entry e;
        e.name = o.value("name").toString();
        e.exec = o.value("url").toString();
        if (!e.exec.startsWith("http://") && !e.exec.startsWith("https://")) {
            e.exec = "http://" + e.exec;
        }
        e.comment = o.value("comment").toString();
        e.icon = o.value("icon").toString();
        if (e.icon.isEmpty()) {
            e.icon = QStringLiteral("zen-browser");
        }
        e.lower_name = e.name.toLower();
        if (!e.name.isEmpty() && !e.exec.isEmpty()) {
            all_.append(e);
        }
    }
}

void EntryModel::load_dotfiles() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);

    QDir config_dir(path);
    if (!config_dir.exists()) {
        return;
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
        all_.append(e);
    }
}

bool EntryModel::parse_desktop(const QString& path, Entry& out) {
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

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {
    cache_.setMaxCost(4 * 1024);

    static const char* const names[] = {
        "application-x-executable",
        "application-x-executable-symbolic",
        "application-x-application",
        "application-default-icon",
        "exec",
        nullptr,
    };
    for (const char* const* n = names; *n; ++n) {
        QIcon i = QIcon::fromTheme(*n);
        if (!i.isNull()) {
            fallback_icon_ = i.pixmap(QSize(24, 24));
            break;
        }
    }
    if (fallback_icon_.isNull()) {
        fallback_icon_ = QPixmap(24, 24);
        fallback_icon_.fill(Qt::transparent);
    }
}

QPixmap IconProvider::requestPixmap(const QString& id, QSize* size, const QSize& requested) {
    const QSize target = requested.isValid() ? requested : QSize(32, 32);
    const QString key = id + '@' + QString::number(target.width()) + 'x' + QString::number(target.height());

    if (QPixmap* cached = cache_.object(key)) {
        if (size) {
            *size = cached->size();
        }
        return *cached;
    }

    QIcon icon;
    if (id.startsWith('/')) {
        if (QFileInfo::exists(id)) {
            icon = QIcon(id);
        }
    } else if (!id.isEmpty()) {
        icon = QIcon::fromTheme(id);
    }

    QPixmap pm;
    if (!icon.isNull()) {
        pm = icon.pixmap(target);
    }
    if (pm.isNull()) {
        pm = fallback_icon_.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (size) {
        *size = pm.size();
    }

    const int cost = qMax(1, pm.width() * pm.height() * pm.depth() / 8 / 1024);
    cache_.insert(key, new QPixmap(pm), cost);
    return pm;
}
