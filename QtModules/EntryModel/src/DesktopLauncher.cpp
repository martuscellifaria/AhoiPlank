#include "DesktopLauncher.h"
#include <QProcess>
#include <QRegularExpression>

namespace {

QString strip_desktop_field_codes(const QString& exec) {
    static const QRegularExpression re(QStringLiteral("%[fFuUdDnNickvm%]"));
    QString out;
    out.reserve(exec.size());
    int last = 0;
    auto it = re.globalMatch(exec);
    while (it.hasNext()) {
        const auto m = it.next();
        out += exec.mid(last, m.capturedStart() - last);
        if (m.capturedLength() == 2 && m.captured().at(1) == u'%') {
            out += u'%';
        }
        last = m.capturedEnd();
    }
    out += exec.mid(last);
    return out;
}

} // namespace

bool DesktopLauncher::launch(const Entry& entry, const QString&, const QString&) {
    const QString exec = strip_desktop_field_codes(entry.exec);
    QStringList argv = QProcess::splitCommand(exec);
    if (argv.isEmpty()) {
        return false;
    }
    return QProcess::startDetached(argv.takeFirst(), argv);
}
