#pragma once

#include <QColor>
#include <QObject>
#include <QString>

class ThemeConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(QColor bg        READ bg        NOTIFY changed)
    Q_PROPERTY(QColor bg_input  READ bg_input  NOTIFY changed)
    Q_PROPERTY(QColor fg        READ fg        NOTIFY changed)
    Q_PROPERTY(QColor fg_dim    READ fg_dim    NOTIFY changed)
    Q_PROPERTY(QColor border    READ border    NOTIFY changed)
    Q_PROPERTY(QColor selection READ selection NOTIFY changed)
    Q_PROPERTY(QColor accent    READ accent    NOTIFY changed)

    Q_PROPERTY(QString font_family      READ font_family      NOTIFY changed)
    Q_PROPERTY(int     font_size_search READ font_size_search NOTIFY changed)
    Q_PROPERTY(int     font_size_title  READ font_size_title  NOTIFY changed)
    Q_PROPERTY(int     font_size_comment READ font_size_comment NOTIFY changed)

    Q_PROPERTY(int radius_window READ radius_window NOTIFY changed)
    Q_PROPERTY(int radius_item   READ radius_item   NOTIFY changed)

    Q_PROPERTY(int screen_width  READ screen_width  NOTIFY changed)
    Q_PROPERTY(int screen_height READ screen_height NOTIFY changed)

public:
    explicit ThemeConfig(QObject *parent = nullptr);

    QColor bg()        const { return bg_; }
    QColor bg_input()  const { return bg_input_; }
    QColor fg()        const { return fg_; }
    QColor fg_dim()    const { return fg_dim_; }
    QColor border()    const { return border_; }
    QColor selection() const { return selection_; }
    QColor accent()    const { return accent_; }

    QString font_family()       const { return font_family_; }
    int     font_size_search()  const { return font_size_search_; }
    int     font_size_title()   const { return font_size_title_; }
    int     font_size_comment() const { return font_size_comment_; }

    int radius_window() const { return radius_window_; }
    int radius_item()   const { return radius_item_; }

    int screen_width()  const { return screen_width_; }
    int screen_height() const { return screen_height_; }

signals:
    void changed();

private:
    QColor bg_        { QStringLiteral("#1f2335") };
    QColor bg_input_  { QStringLiteral("#1a1b26") };
    QColor fg_        { QStringLiteral("#c0caf5") };
    QColor fg_dim_    { QStringLiteral("#a9b1d6") };
    QColor border_    { QStringLiteral("#7dcfff66") };
    QColor selection_ { QStringLiteral("#7aa2f724") };
    QColor accent_    { QStringLiteral("#f7768e") };

    QString font_family_       { QStringLiteral("monospace") };
    int     font_size_search_  { 18 };
    int     font_size_title_   { 14 };
    int     font_size_comment_ { 11 };

    int radius_window_ { 14 };
    int radius_item_   { 8 };

    int screen_width_  { 640 };
    int screen_height_ { 420 };
};
