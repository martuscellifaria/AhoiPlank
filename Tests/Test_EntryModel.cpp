#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <qdir.h>
#include <qlogging.h>
#include <qstandardpaths.h>
#include <qtenvironmentvariables.h>
#include "EntryModel.h"

class TestEntryModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        temp_dir_ = std::make_unique<QTemporaryDir>();
        QVERIFY2(temp_dir_->isValid(), "could not create temp dir");

        const QString apps_dir = temp_dir_->path() + "/applications";
        QVERIFY(QDir().mkpath(apps_dir));

        write_desktop(apps_dir + "/firefox.desktop",
                      "Firefox", "firefox %u", "Browse the web");

        write_desktop(apps_dir + "/terminal.desktop",
                      "Terminal", "foot", "Open a terminal");

        write_desktop(apps_dir + "/editor.desktop",
                      "Editor", "nvim", "Edit text files");

        write_desktop(apps_dir + "/hidden.desktop",
                      "Hidden App", "hidden", "", true);

        QFile link_file(apps_dir + "/link.desktop");
        QVERIFY(link_file.open(QIODevice::WriteOnly));
        link_file.write("[Desktop Entry]\n"
                        "Type=Link\n"
                        "Name=Some Link\n"
                        "URL=https://example.com\n");
        link_file.close();

        qputenv("XDG_DATA_HOME", temp_dir_->path().toUtf8());
        qputenv("XDG_DATA_DIRS", temp_dir_->path().toUtf8());
    }

    void init() {
        model_ = std::make_unique<EntryModel>();
    }

    void rowCount_matchesExpectedEntries() {
        QCOMPARE(model_->rowCount(), 3);
    }

    void data_returnsExpectedRoles() {
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Editor"));
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 2).toString(),
                 QStringLiteral("nvim"));
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 3).toString(),
                 QStringLiteral("Edit text files"));
    }

    void data_invalidIndexReturnsEmpty() {
        QVERIFY(model_->data(model_->index(-1), Qt::UserRole + 1).isNull());
        QVERIFY(model_->data(model_->index(999), Qt::UserRole + 1).isNull());
    }

    void filter_emptyKeepsAll() {
        model_->set_filter("");
        QCOMPARE(model_->rowCount(), 3);
    }

    void filter_noMatchReturnsZero() {
        model_->set_filter(QStringLiteral("zzzzzzzzzzzzzzzzzz"));
        QCOMPARE(model_->rowCount(), 0);
    }

    void filter_exactSubstring() {
        model_->set_filter(QStringLiteral("fire"));
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Firefox"));
    }

    void filter_caseInsensitive() {
        model_->set_filter(QStringLiteral("TERM"));
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Terminal"));
    }

    void filter_fuzzyAcronym() {
        model_->set_filter(QStringLiteral("term"));
        QVERIFY(model_->rowCount() >= 1);
        bool found = false;
        for (int i = 0; i < model_->rowCount(); ++i) {
            if (model_->data(model_->index(i), Qt::UserRole + 1).toString()
                == QStringLiteral("Terminal")) {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }

    void filter_multiTerm() {
        model_->set_filter(QStringLiteral("term edit"));
        QCOMPARE(model_->rowCount(), 0);
    }

    void filter_repeatedCallsAreStable() {
        for (int i = 0; i < 100; ++i) {
            model_->set_filter(QStringLiteral(""));
            QCOMPARE(model_->rowCount(), 3);
        }
    }

    void filter_toggleBackAndForth() {
        for (int i = 0; i < 50; ++i) {
            model_->set_filter(QStringLiteral("fire"));
            QCOMPARE(model_->rowCount(), 1);
            model_->set_filter(QStringLiteral(""));
            QCOMPARE(model_->rowCount(), 3);
        }
    }

    void mode_standardIsDefault() {
        QCOMPARE(model_->mode(), QStringLiteral("standard"));
    }

    void mode_setModeChangesMode() {
        model_->set_mode(QStringLiteral("bookmark"));
        QCOMPARE(model_->mode(), QStringLiteral("bookmark"));
        model_->set_mode(QStringLiteral("standard"));
        QCOMPARE(model_->mode(), QStringLiteral("standard"));
    }

    void mode_switchingDoesNotCrash() {
        model_->set_mode(QStringLiteral("bookmark"));
        model_->set_mode(QStringLiteral("dotfiles"));
        model_->set_mode(QStringLiteral("standard"));
        QVERIFY(model_->rowCount() >= 0);
    }

    void mode_sameModeIsNoOp() {
        model_->set_mode(QStringLiteral("standard"));
        QCOMPARE(model_->rowCount(), 3);
    }

private:
    static void write_desktop(const QString& path,
                              const QString& name,
                              const QString& exec,
                              const QString& comment,
                              bool no_display = false) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QByteArray content = "[Desktop Entry]\n"
                             "Type=Application\n"
                             "Name=" + name.toUtf8() + "\n"
                             "Exec=" + exec.toUtf8() + "\n";
        if (!comment.isEmpty()) {
            content += "Comment=" + comment.toUtf8() + "\n";
        }
        if (no_display) {
            content += "NoDisplay=true\n";
        }
        f.write(content);
        f.close();
    }

    std::unique_ptr<QTemporaryDir> temp_dir_;
    std::unique_ptr<EntryModel> model_;
};

QTEST_MAIN(TestEntryModel)
#include "Test_EntryModel.moc"
