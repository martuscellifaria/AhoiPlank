#include "BookmarkLauncher.h"
#include <QProcess>

bool BookmarkLauncher::launch(const Entry& entry, const QString&, const QString&) {
    return QProcess::startDetached("xdg-open", { entry.exec });
}
