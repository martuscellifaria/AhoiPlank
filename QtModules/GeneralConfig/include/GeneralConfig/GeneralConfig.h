#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <qhashfunctions.h>
#include <qtmetamacros.h>

class GeneralConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(QColor bg READ bg NOTIFY changed)
    Q_PROPERTY(QColor bg_input READ bg_input  NOTIFY changed)
    Q_PROPERTY(QColor fg READ fg NOTIFY changed)
    Q_PROPERTY(QColor fg_dim READ fg_dim NOTIFY changed)
    Q_PROPERTY(QColor border READ border NOTIFY changed)
    Q_PROPERTY(QColor selection READ selection NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)

    Q_PROPERTY(QString font_family READ font_family NOTIFY changed)
    Q_PROPERTY(int font_size_search READ font_size_search NOTIFY changed)
    Q_PROPERTY(int font_size_title  READ font_size_title  NOTIFY changed)
    Q_PROPERTY(int font_size_comment READ font_size_comment NOTIFY changed)
    Q_PROPERTY(bool font_bold READ font_bold NOTIFY changed)

    Q_PROPERTY(int radius_window READ radius_window NOTIFY changed)
    Q_PROPERTY(int radius_item   READ radius_item   NOTIFY changed)
    Q_PROPERTY(int border_width  READ border_width  NOTIFY changed)

    Q_PROPERTY(int screen_width  READ screen_width  NOTIFY changed)
    Q_PROPERTY(int screen_height READ screen_height NOTIFY changed)

    Q_PROPERTY(QString vertical_position READ vertical_position NOTIFY changed)

    Q_PROPERTY(int columns READ columns NOTIFY changed)

    Q_PROPERTY(bool status_bar_visible READ status_bar_visible NOTIFY changed)
    Q_PROPERTY(QString status_bar_position READ status_bar_position NOTIFY changed)
    Q_PROPERTY(QColor status_bar_bg READ status_bar_bg NOTIFY changed)
    Q_PROPERTY(QColor status_bar_fg READ status_bar_fg NOTIFY changed)
    Q_PROPERTY(QColor status_bar_pill READ status_bar_pill NOTIFY changed)
    Q_PROPERTY(int status_bar_height READ status_bar_height NOTIFY changed)
    Q_PROPERTY(int status_bar_gap READ status_bar_gap NOTIFY changed)
    Q_PROPERTY(int status_bar_pill_padding READ status_bar_pill_padding NOTIFY changed)

    Q_PROPERTY(int layout_margin READ layout_margin NOTIFY changed)
    Q_PROPERTY(int layout_spacing READ layout_spacing NOTIFY changed)
    Q_PROPERTY(int search_padding READ search_padding NOTIFY changed)

    Q_PROPERTY(int item_height READ item_height NOTIFY changed)
    Q_PROPERTY(int item_padding READ item_padding NOTIFY changed)
    Q_PROPERTY(int icon_size READ icon_size NOTIFY changed)
    Q_PROPERTY(int icon_spacing READ icon_spacing NOTIFY changed)

    Q_PROPERTY(QString text_editor READ text_editor NOTIFY changed)
    Q_PROPERTY(QString terminal READ terminal NOTIFY changed)

public:
    explicit GeneralConfig(QObject *parent = nullptr);

    QColor bg() const { return bg_; }
    QColor bg_input() const { return bg_input_; }
    QColor fg() const { return fg_; }
    QColor fg_dim() const { return fg_dim_; }
    QColor border() const { return border_; }
    QColor selection() const { return selection_; }
    QColor accent() const { return accent_; }

    QString font_family() const { return font_family_; }
    int font_size_search() const { return font_size_search_; }
    int font_size_title() const { return font_size_title_; }
    int font_size_comment() const { return font_size_comment_; }
    bool font_bold() const { return font_bold_; }

    int radius_window() const { return radius_window_; }
    int radius_item()   const { return radius_item_; }
    int border_width()  const { return border_width_; }

    int screen_width()  const { return screen_width_; }
    int screen_height() const { return screen_height_; }
    QString vertical_position() const { return vertical_position_; }

    int columns() const { return columns_; }
    bool status_bar_visible() const { return status_bar_visible_; }
    QString status_bar_position() const { return status_bar_position_; }
    QColor status_bar_bg() const { return status_bar_bg_; }
    QColor status_bar_fg() const { return status_bar_fg_; }
    QColor status_bar_pill() const { return status_bar_pill_; }
    int status_bar_height() const { return status_bar_height_; }
    int status_bar_gap() const { return status_bar_gap_; }
    int status_bar_pill_padding() const { return status_bar_pill_padding_; }

    int layout_margin() const { return layout_margin_; }
    int layout_spacing() const { return layout_spacing_; }
    int search_padding() const { return search_padding_; }

    int item_height() const { return item_height_; }
    int item_padding() const { return item_padding_; }
    int icon_size() const { return icon_size_; }
    int icon_spacing() const { return icon_spacing_; }

    QString text_editor() const { return text_editor_; }
    QString terminal() const { return terminal_; }

    void reload();

signals:
    void changed();

private:
    QColor bg_ { QStringLiteral("#1f2335") };
    QColor bg_input_ { QStringLiteral("#1a1b26") };
    QColor fg_ { QStringLiteral("#c0caf5") };
    QColor fg_dim_ { QStringLiteral("#a9b1d6") };
    QColor border_ { QStringLiteral("#667dcfff") };
    QColor selection_ { QStringLiteral("#247aa2f7") };
    QColor accent_ { QStringLiteral("#f7768e") };

    QString font_family_ { QStringLiteral("monospace") };
    int font_size_search_ { 18 };
    int font_size_title_ { 14 };
    int font_size_comment_ { 11 };
    bool font_bold_ { false };

    int radius_window_ { 14 };
    int radius_item_ { 8 };
    int border_width_ { 1 };

    int screen_width_ { 640 };
    int screen_height_ { 420 };
    QString vertical_position_ = "center";

    int columns_ { 1 };

    bool status_bar_visible_ { false };
    QString status_bar_position_ = "bottom";
    QColor status_bar_bg_ { QStringLiteral("#1a1b26") };
    QColor status_bar_fg_ { QStringLiteral("#c0caf5") };
    QColor status_bar_pill_ { QStringLiteral("#7dcfff") };
    int status_bar_height_ { 20 };
    int status_bar_gap_ { 12 };
    int status_bar_pill_padding_ { 14 };

    int layout_margin_ { 16 };
    int layout_spacing_ { 12 };
    int search_padding_ { 8 };

    int item_height_ { 44 };
    int item_padding_ { 12 };
    int icon_size_ { 24 };
    int icon_spacing_ { 10 };

    QString text_editor_ = "vim";
    QString terminal_ = "";
};
