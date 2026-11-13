#include "IconProvider.h"
#include <QFileInfo>
#include <QIcon>
#include <QPixmap>

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {
    cache_.setMaxCost(4 * 1024);

    static const char* const names[] = {
        "application-x-executable",
        "application-x-executable-symbolic",
        "application-x-application",
        "application-default-icon",
        "exec",
        nullptr,
    };
    for (const char* const* n = names; *n; ++n) {
        QIcon i = QIcon::fromTheme(*n);
        if (!i.isNull()) {
            fallback_icon_ = i.pixmap(QSize(24, 24));
            break;
        }
    }
    if (fallback_icon_.isNull()) {
        fallback_icon_ = QPixmap(24, 24);
        fallback_icon_.fill(Qt::transparent);
    }
}

QPixmap IconProvider::requestPixmap(const QString& id, QSize* size, const QSize& requested) {
    const QSize target = requested.isValid() ? requested : QSize(32, 32);
    const QString key = id + '@' + QString::number(target.width()) + 'x' + QString::number(target.height());

    if (QPixmap* cached = cache_.object(key)) {
        if (size) {
            *size = cached->size();
        }
        return *cached;
    }

    QIcon icon;
    if (id.startsWith('/')) {
        if (QFileInfo::exists(id)) {
            icon = QIcon(id);
        }
    } else if (!id.isEmpty()) {
        icon = QIcon::fromTheme(id);
    }

    QPixmap pm;
    if (!icon.isNull()) {
        pm = icon.pixmap(target);
    }
    if (pm.isNull()) {
        pm = fallback_icon_.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (size) {
        *size = pm.size();
    }

    const int cost = qMax(1, pm.width() * pm.height() * pm.depth() / 8 / 1024);
    cache_.insert(key, new QPixmap(pm), cost);
    return pm;
}
