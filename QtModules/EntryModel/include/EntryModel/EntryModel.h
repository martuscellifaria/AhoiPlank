#pragma once
#include <QAbstractListModel>
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

signals:
    void dismissed();

private:
    void load_desktop_files();

    struct Entry {
        QString name;
        QString exec;
        QString comment;
        QString icon;
        QString lower_name;
    };
    static bool parse_desktop(const QString& path, Entry& out);

    QVector<Entry> all_;
    QVector<Entry> filtered_;
};


class IconProvider : public QQuickImageProvider {
public:
    IconProvider();
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requested) override;
private:
    QHash<QString, QPixmap> cache_;
    QPixmap fallback_icon_;
};
