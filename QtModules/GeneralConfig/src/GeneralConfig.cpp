#include "GeneralConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QColor parse_color(const QString &s) {
    QString str = s.trimmed();
    if (str.startsWith('#')) {
        str = str.mid(1);
    }

    if (str.length() == 8) {
        bool ok = false;
        const uint rgba = str.toUInt(&ok, 16);
        if (ok) {
            return QColor(
                static_cast<int>((rgba >> 24) & 0xFFu),
                static_cast<int>((rgba >> 16) & 0xFFu),
                static_cast<int>((rgba >> 8)  & 0xFFu),
                static_cast<int>( rgba        & 0xFFu)
            );
        }
        return QColor();
    }

    return QColor(str.isEmpty() ? s : "#" + str);
}

QColor color_or(const QJsonObject &obj, const char *key, const QColor &fallback) {
    const QString s = obj.value(QLatin1String(key)).toString();
    if (s.isEmpty()) {
        return fallback;
    }
    const QColor c = parse_color(s);
    if (!c.isValid()) {
        return fallback;
    }
    return c;
}

int int_or(const QJsonObject &obj, const char *key, int fallback) {
    const auto v = obj.value(QLatin1String(key));
    if (!v.isDouble()) {
        return fallback;
    }
    return v.toInt();
}

QString string_or(const QJsonObject &obj, const char *key, const QString &fallback) {
    const auto v = obj.value(QLatin1String(key));
    if (!v.isString()) {
        return fallback;
    }
    return v.toString();
}

QString config_path() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + "/ahoiplank/config.json";
}

} // namespace

GeneralConfig::GeneralConfig(QObject *parent) : QObject(parent) {
    reload();
}

void GeneralConfig::reload() {
    QColor bg         { QStringLiteral("#1f2335") };
    QColor bg_input   { QStringLiteral("#1a1b26") };
    QColor fg         { QStringLiteral("#c0caf5") };
    QColor fg_dim     { QStringLiteral("#a9b1d6") };
    QColor border     { QStringLiteral("#7dcfff66") };
    QColor selection  { QStringLiteral("#7aa2f724") };
    QColor accent     { QStringLiteral("#f7768e") };

    QString font_family       { QStringLiteral("monospace") };
    int     font_size_search  { 18 };
    int     font_size_title   { 14 };
    int     font_size_comment { 11 };

    int radius_window { 14 };
    int radius_item   { 8 };

    int screen_width  { 640 };
    int screen_height { 420 };
    QString vertical_position = "center";
    QString text_editor = "vim";
    QString text_editor_terminal = "";

    QFile f(config_path());
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        if (!root.isEmpty()) {
            const QJsonObject style = root.value("style").toObject();
            const QJsonObject colors = style.value("colors").toObject();

            bg  = color_or(colors, "bg", bg);
            bg_input = color_or(colors, "bg_input", bg_input);
            fg = color_or(colors, "fg", fg);
            fg_dim = color_or(colors, "fg_dim", fg_dim);
            border = color_or(colors, "border", border);
            selection = color_or(colors, "selection", selection);
            accent = color_or(colors, "accent", accent);

            text_editor = string_or(root, "text_editor", text_editor);
            text_editor_terminal = string_or(root, "text_editor_terminal", text_editor_terminal);

            font_family = string_or(style, "font_family", font_family);
            font_size_search = int_or(style, "font_size_search", font_size_search);
            font_size_title = int_or(style, "font_size_title", font_size_title);
            font_size_comment = int_or(style, "font_size_comment", font_size_comment);

            radius_window = int_or(style, "radius_window", radius_window);
            radius_item = int_or(style, "radius_item",   radius_item);

            screen_width  = int_or(style, "screen_width",  screen_width);
            screen_height = int_or(style, "screen_height", screen_height);

            const QJsonValue vp = style.value("vertical_position");
            if (vp.isString()) {
                vertical_position = vp.toString();
            }
            else if (vp.isDouble()) {
                vertical_position = QString::number(vp.toInt());
            }
        }
    }

    bg_ = bg;
    bg_input_ = bg_input;
    fg_ = fg;
    fg_dim_ = fg_dim;
    border_ = border;
    selection_ = selection;
    accent_ = accent;

    font_family_ = font_family;
    font_size_search_ = font_size_search;
    font_size_title_ = font_size_title;
    font_size_comment_ = font_size_comment;

    radius_window_ = radius_window;
    radius_item_ = radius_item;

    screen_width_ = screen_width;
    screen_height_ = screen_height;
    vertical_position_ = vertical_position;

    text_editor_ = text_editor;
    text_editor_terminal_ = text_editor_terminal;

    emit changed();
}
