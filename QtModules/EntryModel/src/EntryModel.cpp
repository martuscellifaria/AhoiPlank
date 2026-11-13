#include "EntryModel.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <print>

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

QString strip_desktop_field_codes(const QString& exec) {
    static const QRegularExpression re(QStringLiteral("%[fFuUdDnNickvm%]"));
    QString out;
    out.reserve(exec.size());
    int last = 0;
    auto it = re.globalMatch(exec);
    while (it.hasNext()) {
        const auto m = it.next();
        out += exec.mid(last, m.capturedStart() - last);
        if (m.capturedLength() == 2 && m.captured().at(1) == u'%') {
            out += u'%';
        }
        last = m.capturedEnd();
    }
    out += exec.mid(last);
    return out;
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
    all_.clear();
    filtered_idx_.clear();

    switch (mode_) {
        default:
        case Mode::Standard:
        {
            load_desktop_files();
            break;
        }
        case Mode::Bookmarks:
        {
            load_bookmarks();
            break;
        }
        case Mode::Dotfiles:
        {
            load_dotfiles();
            break;
        }
        case Mode::SSH:
        {
            load_ssh();
            break;
        }
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
    const Entry& entry = all_.at(filtered_idx_.at(row));

    switch (mode_) {
        default:
        case Mode::Standard:
            return launch_standard(entry);
        case Mode::Bookmarks:
            return launch_bookmark(entry);
        case Mode::Dotfiles:
            return launch_dotfile(entry);
        case Mode::SSH:
            return launch_ssh(entry);
    }
}

bool EntryModel::launch_standard(const Entry &entry) {
    const QString exec = strip_desktop_field_codes(entry.exec);
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

bool EntryModel::launch_bookmark(const Entry &entry) {
    const bool ok = QProcess::startDetached("xdg-open", { entry.exec });
    if (ok) {
        emit dismissed();
    }
    return ok;
}

bool EntryModel::launch_dotfile(const Entry &entry) {
    const QString terminal = TerminalLauncher::find_terminal(terminal_);
    if (terminal.isEmpty()) {
        std::println(stderr, "dotfiles: no terminal found, set terminal in config.json");
        return false;
    }

    const QString script = QStringLiteral("cd %1 && exec %2 .")
        .arg(TerminalLauncher::shell_quote(entry.exec), text_editor_);

    const bool ok = TerminalLauncher::run_in_terminal_argv(
        terminal,
        { "sh", "-c", script });

    if (ok) {
        emit dismissed();
    }
    return ok;
}

bool EntryModel::launch_ssh(const Entry &entry) {
    const QString terminal = TerminalLauncher::find_terminal(terminal_);
    if (terminal.isEmpty()) {
        std::println(stderr, "ssh: no terminal found, set terminal in config.json");
        return false;
    }

    const bool ok = TerminalLauncher::run_in_terminal_argv(
        terminal,
        { "ssh", entry.exec });

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

    QJsonParseError perr{};
    const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "bookmarks: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return;
    }
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

void EntryModel::load_ssh() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + "/ahoiplank/ssh.json";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError perr{};
    const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &perr).object();
    if (perr.error != QJsonParseError::NoError) {
        std::println(stderr, "ssh: {} is not valid json ({})",
                     path.toStdString(), perr.errorString().toStdString());
        return;
    }
    if (root.isEmpty()) {
        return;
    }

    const QJsonArray arr = root.value("ssh").toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        Entry e;
        e.name = o.value("name").toString();
        e.exec = o.value("user").toString() + "@" + o.value("host").toString();
        e.icon = o.value("icon").toString();
        if (e.icon.isEmpty()) {
            e.icon = QStringLiteral("network-server");
        }
        e.lower_name = e.name.toLower();
        if (!e.name.isEmpty() && !e.exec.isEmpty()) {
            all_.append(e);
        }
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
