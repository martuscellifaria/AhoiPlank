#pragma once
#include <QAbstractListModel>
#include <QCache>
#include <QHash>
#include <QString>
#include <QVector>
#include <QQuickImageProvider>
#include <qhashfunctions.h>


class EntryModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit EntryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void set_filter(const QString& query);
    Q_INVOKABLE bool launch(int row);
    Q_INVOKABLE void quit();
    Q_INVOKABLE void dismiss();
    Q_INVOKABLE void set_mode(const QString& mode);
    Q_INVOKABLE void reload();

    QString mode() const { return mode_; }

    void request_reset();
    void set_text_editor(const QString& editor) { text_editor_ = editor; }
    void set_text_editor_terminal(const QString& terminal) { text_editor_terminal_ = terminal; }

signals:
    void dismissed();
    void resetRequested();
    void modeChanged();

private:
    void load_desktop_files();
    void load_bookmarks();
    void load_dotfiles();
    void rebuild();

    struct Entry {
        QString name;
        QString exec;
        QString comment;
        QString icon;
        QString lower_name;
    };
    static bool parse_desktop(const QString& path, Entry& out);

    QString mode_ = QStringLiteral("standard");
    QString text_editor_ = QStringLiteral("vim");
    QString text_editor_terminal_ = QStringLiteral("");

    QVector<Entry> all_;
    QVector<int>   filtered_idx_;
    QVector<QPair<double, int>> scored_buf_;
    QString current_filter_;
};


class IconProvider : public QQuickImageProvider {
public:
    IconProvider();
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requested) override;
private:
    QCache<QString, QPixmap> cache_;
    QPixmap fallback_icon_;
};
