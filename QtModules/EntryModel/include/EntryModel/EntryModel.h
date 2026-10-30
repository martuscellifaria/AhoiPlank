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

    int rowCount(const QModelIndex& parent = {}) const override; // Qt base class virtual function, therefore camelCase.
    QVariant data(const QModelIndex& idx, int role) const override;
    QHash<int, QByteArray> roleNames() const override; // same

    Q_INVOKABLE void set_filter(const QString& query);
    Q_INVOKABLE bool launch(int row);
    Q_INVOKABLE void quit();

private:
    void load_desktop_files();

    struct entry {
        QString name;
        QString exec;
        QString comment;
        QString icon;
        QString lower_name;
    };

    QVector<entry> all_;
    QVector<entry> filtered_;
};

class IconProvider : public QQuickImageProvider {
public:
    IconProvider();

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requested) override;
};

