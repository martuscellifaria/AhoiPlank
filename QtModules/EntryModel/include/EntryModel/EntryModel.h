#pragma once
#include "Entry.h"
#include "EntryLauncher.h"
#include "EntrySource.h"
#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QStringView>
#include <QVector>
#include <QProcess>
#include <QStandardPaths>
#include <array>
#include <cstdint>
#include <memory>
#include <qcontainerfwd.h>

namespace TerminalLauncher {

inline QString find_terminal(const QString& configured = {}) {
    if (!configured.isEmpty()) {
        return configured;
    }
    if (const QString env = qEnvironmentVariable("TERMINAL"); !env.isEmpty()) {
        return env;
    }
    static const char* const candidates[] = {
        "alacritty", "foot", "kitty", "wezterm", "ghostty",
        "konsole", "gnome-terminal", "xfce4-terminal", "xterm",
        nullptr,
    };
    for (const char* const* c = candidates; *c; ++c) {
        const QString name = QString::fromLatin1(*c);
        if (!QStandardPaths::findExecutable(name).isEmpty()) {
            return name;
        }
    }
    return {};
}

inline QString shell_quote(const QString& s) {
    if (s.isEmpty()) {
        return QStringLiteral("''");
    }
    QString out;
    out.reserve(s.size() + 2);
    out += u'\'';
    for (const QChar c : s) {
        if (c == u'\'') {
            out += QStringLiteral("'\\''");
        }
        else {
            out += c;
        }
    }
    out += u'\'';
    return out;
}

inline QStringList terminal_flags(const QString& terminal) {
    if (terminal.endsWith("gnome-terminal")) {
        return { "--" };
    }
    if (terminal.endsWith("wezterm")) {
        return { "start", "--" };
    }
    if (terminal.endsWith("xfce4-terminal")) {
        return { "-x" };
    }
    return { "-e" };
}

inline bool run_in_terminal_argv(const QString& terminal,
                                 const QStringList& argv) {
    QStringList args = terminal_flags(terminal);
    args += argv;
    return QProcess::startDetached(terminal, args);
}

} // namespace TerminalLauncher

class EntryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString modeString READ mode_string NOTIFY modeChanged)

public:
    enum class Mode : std::uint8_t {
        Standard,
        Bookmarks,
        Dotfiles,
        SSH
    };

    static inline QString mode_to_string(Mode m) {
        switch (m) {
            case Mode::Standard:  return QStringLiteral("standard");
            case Mode::Bookmarks: return QStringLiteral("bookmark");
            case Mode::Dotfiles:  return QStringLiteral("dotfiles");
            case Mode::SSH:       return QStringLiteral("ssh");
        }
        return QStringLiteral("standard");
    }

    static inline Mode mode_from_string(QStringView s) {
        if (s == u"bookmark" || s == u"bookmarks") { return Mode::Bookmarks; }
        if (s == u"dotfiles") { return Mode::Dotfiles; }
        if (s == u"ssh") { return Mode::SSH; }
        return Mode::Standard;
    }

    explicit EntryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void set_filter(const QString& query);
    Q_INVOKABLE bool launch(int row);
    Q_INVOKABLE void quit();
    Q_INVOKABLE void dismiss();
    Q_INVOKABLE void set_mode(const Mode& mode);
    Q_INVOKABLE void reload();

    Mode mode() const { return mode_; }
    void request_reset();
    void set_text_editor(const QString& editor) { text_editor_ = editor; }
    void set_terminal(const QString& terminal) { terminal_ = terminal; }
    QString mode_string() const { return mode_to_string(mode_); }

signals:
    void dismissed();
    void resetRequested();
    void modeChanged();

private:
    void rebuild();
    void apply_filter_diff(const QVector<int>& new_idx);

    Mode mode_ = Mode::Standard;
    QString text_editor_ = QStringLiteral("vim");
    QString terminal_ = QStringLiteral("");

    QVector<Entry> all_;
    QVector<int> filtered_idx_;
    struct Scored {
        double score; int idx;
    };
    QVector<Scored> scored_buf_;
    QString current_filter_;

    std::array<std::unique_ptr<EntrySource>, 4> sources_;
    std::array<std::unique_ptr<EntryLauncher>, 4> launchers_;
};
