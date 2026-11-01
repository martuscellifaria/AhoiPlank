#include "ThemeConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QColor color_or(const QJsonObject &obj, const char *key, const QColor &fallback) {
    const QString s = obj.value(QLatin1String(key)).toString();
    if (s.isEmpty()) {
        return fallback;
    }
    const QColor c(s);
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

ThemeConfig::ThemeConfig(QObject *parent) : QObject(parent) {
    QFile f(config_path());
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    if (root.isEmpty()) {
        return;
    }

    const QJsonObject colors = root.value("colors").toObject();

    bg_        = color_or(colors, "bg",        bg_);
    bg_input_  = color_or(colors, "bg_input",  bg_input_);
    fg_        = color_or(colors, "fg",        fg_);
    fg_dim_    = color_or(colors, "fg_dim",    fg_dim_);
    border_    = color_or(colors, "border",    border_);
    selection_ = color_or(colors, "selection", selection_);
    accent_    = color_or(colors, "accent",    accent_);

    font_family_       = string_or(root, "font_family",       font_family_);
    font_size_search_  = int_or(root, "font_size_search",     font_size_search_);
    font_size_title_   = int_or(root, "font_size_title",      font_size_title_);
    font_size_comment_ = int_or(root, "font_size_comment",    font_size_comment_);

    radius_window_ = int_or(root, "radius_window", radius_window_);
    radius_item_   = int_or(root, "radius_item",   radius_item_);

    screen_width_  = int_or(root, "screen_width",  screen_width_);
    screen_height_ = int_or(root, "screen_height", screen_height_);
}
