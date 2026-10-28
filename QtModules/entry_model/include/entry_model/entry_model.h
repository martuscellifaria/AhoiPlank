#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QVector>
#include <QQuickImageProvider>
#include <qhashfunctions.h>


class entry_model : public QAbstractListModel {
    Q_OBJECT

public:
    explicit entry_model(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

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

class icon_provider : public QQuickImageProvider {
public:
    icon_provider();

    QPixmap requestPixmap(const QString& id, QSize* size,
                          const QSize& requested) override;
};

