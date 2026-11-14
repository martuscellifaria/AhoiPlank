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

bool bool_or(const QJsonObject &obj, const char *key, bool fallback) {
    const auto v = obj.value(QLatin1String(key));
    if (v.isBool())  {
        return v.toBool();
    }
    if (v.isDouble()) {
        return v.toInt() != 0;
    }
    if (v.isString()) {
        const auto s = v.toString().trimmed().toLower();
        if (s == "true"  || s == "1" || s == "yes" || s == "on")  {
            return true;
        }
        if (s == "false" || s == "0" || s == "no"  || s == "off") {
            return false;
        }
    }
    return fallback;
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
    QColor border     { QStringLiteral("#667dcfff") };
    QColor selection  { QStringLiteral("#247aa2f7") };
    QColor accent     { QStringLiteral("#f7768e") };

    QString font_family       { QStringLiteral("monospace") };
    int font_size_search  { 18 };
    int font_size_title   { 14 };
    int font_size_comment { 11 };
    bool font_bold { false };

    int radius_window { 14 };
    int radius_item   { 8 };
    int border_width   { 1 };

    int screen_width  { 640 };
    int screen_height { 420 };
    int columns { 1 };
    QString vertical_position = "center";

    bool status_bar_visible { false };
    QString status_bar_position = "bottom";
    QColor status_bar_bg { QStringLiteral("#1a1b26") };
    QColor status_bar_fg { QStringLiteral("#c0caf5") };
    QColor status_bar_pill { QStringLiteral("#7dcfff") };
    int status_bar_height { 20 };
    int status_bar_gap { 12 };
    int status_bar_pill_padding { 14 };

    int layout_margin { 16 };
    int layout_spacing { 12 };
    int search_padding { 8 };

    int item_height { 44 };
    int item_padding { 12 };
    int icon_size { 24 };
    int icon_spacing { 10 };

    QString text_editor = "vim";
    QString terminal = "";

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
            status_bar_bg   = color_or(colors, "status_bar_bg",   status_bar_bg);
            status_bar_fg   = color_or(colors, "status_bar_fg",   status_bar_fg);
            status_bar_pill = color_or(colors, "status_bar_pill", status_bar_pill);

            text_editor = string_or(root, "text_editor", text_editor);
            terminal = string_or(root, "terminal", terminal);

            font_family = string_or(style, "font_family", font_family);
            font_size_search = int_or(style, "font_size_search", font_size_search);
            font_size_title = int_or(style, "font_size_title", font_size_title);
            font_size_comment = int_or(style, "font_size_comment", font_size_comment);
            font_bold = bool_or(style, "font_bold", font_bold);

            radius_window = int_or(style, "radius_window", radius_window);
            radius_item = int_or(style, "radius_item", radius_item);
            border_width = int_or(style, "border_width", border_width);

            screen_width  = int_or(style, "screen_width",  screen_width);
            screen_height = int_or(style, "screen_height", screen_height);

            status_bar_visible = bool_or(style, "status_bar_visible", status_bar_visible);
            status_bar_position = string_or(style, "status_bar_position", status_bar_position);
            status_bar_height = int_or(style, "status_bar_height", status_bar_height);
            status_bar_gap = int_or(style, "status_bar_gap", status_bar_gap);
            status_bar_pill_padding = int_or(style, "status_bar_pill_padding", status_bar_pill_padding);

            layout_margin = int_or(style, "layout_margin", layout_margin);
            layout_spacing = int_or(style, "layout_spacing", layout_spacing);
            search_padding = int_or(style, "search_padding", search_padding);

            item_height = int_or(style, "item_height", item_height);
            item_padding = int_or(style, "item_padding", item_padding);
            icon_size = int_or(style, "icon_size", icon_size);
            icon_spacing = int_or(style, "icon_spacing", icon_spacing);

            columns = int_or(style, "columns", columns);

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
    font_bold_ = font_bold;

    radius_window_ = radius_window;
    radius_item_ = radius_item;
    border_width_ = border_width;

    screen_width_ = screen_width;
    screen_height_ = screen_height;
    vertical_position_ = vertical_position;
    columns_ = columns;

    status_bar_visible_ = status_bar_visible;
    status_bar_position_ = status_bar_position;
    status_bar_bg_ = status_bar_bg;
    status_bar_fg_ = status_bar_fg;
    status_bar_pill_ = status_bar_pill;
    status_bar_height_ = status_bar_height;
    status_bar_gap_ = status_bar_gap;
    status_bar_pill_padding_ = status_bar_pill_padding;

    layout_margin_ = layout_margin;
    layout_spacing_ = layout_spacing;
    search_padding_ = search_padding;

    item_height_ = item_height;
    item_padding_ = item_padding;
    icon_size_ = icon_size;
    icon_spacing_ = icon_spacing;

    text_editor_ = text_editor;
    terminal_ = terminal;

    emit changed();
}
