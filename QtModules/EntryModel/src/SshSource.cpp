#include "SshSource.h"
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QVector>

QVector<Entry> SshSource::load() {
    QVector<Entry> out;
    QSet<QString> seen;

    const QString sshDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.ssh";

    auto addEntry = [&](const QString& name, const QString& target, const QString& comment = {}) {
        if (name.isEmpty() || target.isEmpty() || seen.contains(target)) {
            return;
        }
        seen.insert(target);

        Entry e;
        e.name = name;
        e.exec = target;
        e.id = target;
        e.comment = comment.isEmpty() ? target : comment;
        e.icon = QStringLiteral("network-server");
        e.lower_name = e.name.toLower();
        out.append(std::move(e));
    };

    QFile configFile(sshDir + "/config");
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        static const QRegularExpression hostRe(R"(^\s*Host\s+(.+)$)", QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression userRe(R"(^\s*User\s+(\S+))", QRegularExpression::CaseInsensitiveOption);

        QStringList currentAliases;
        QString currentUser;

        auto flushBlock = [&]() {
            for (const QString& alias : currentAliases) {
                if (alias.contains('*') || alias.contains('?') || alias.startsWith('!')) {
                    continue;
                }
                const QString target = currentUser.isEmpty() ? alias : (currentUser + "@" + alias);
                addEntry(alias, target, QStringLiteral("SSH Config: ") + target);
            }
            currentAliases.clear();
            currentUser.clear();
        };

        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }

            if (const auto m = hostRe.match(line); m.hasMatch()) {
                flushBlock();
                currentAliases = m.captured(1).split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
            }
            else if (const auto m = userRe.match(line); m.hasMatch()) {
                currentUser = m.captured(1);
            }
        }
        flushBlock();
    }

    QFile knownHostsFile(sshDir + "/known_hosts");
    if (knownHostsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&knownHostsFile);
        static const QRegularExpression bracketRe(R"(^\[([^\]]+)\]:(\d+)$)");
        static const QRegularExpression spaceRe(R"(\s+)");

        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#') || line.startsWith('|') || line.startsWith('@')) {
                continue;
            }

            const QString hostField = line.section(spaceRe, 0, 0);
            const QStringList hosts = hostField.split(',', Qt::SkipEmptyParts);

            for (const QString& h : hosts) {
                if (h.startsWith('!')) {
                    continue;
                }

                if (const auto m = bracketRe.match(h); m.hasMatch()) {
                    const QString host = m.captured(1);
                    const QString port = m.captured(2);
                    addEntry(host + ":" + port, host + " -p " + port, QStringLiteral("Visited host"));
                }
                else {
                    addEntry(h, h, QStringLiteral("Visited host"));
                }
            }
        }
    }

    return out;
}
