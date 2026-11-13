#pragma once
#include <QCache>
#include <QPixmap>
#include <QString>
#include <QQuickImageProvider>

class IconProvider : public QQuickImageProvider {
public:
    IconProvider();
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requested) override;
private:
    QCache<QString, QPixmap> cache_;
    QPixmap fallback_icon_;
};
