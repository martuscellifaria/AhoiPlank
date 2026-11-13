#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
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
        qputenv("XDG_CONFIG_HOME", temp_dir_->path().toUtf8());
    }

    void init() {
        model_ = std::make_unique<EntryModel>();
    }

    void cleanup() {
        model_.reset();
        if (temp_dir_ && temp_dir_->isValid()) {
            QFile::remove(ssh_config_path());
        }
        qunsetenv("TERMINAL");
    }

    void cleanupTestCase() {
        qunsetenv("XDG_DATA_HOME");
        qunsetenv("XDG_DATA_DIRS");
        qunsetenv("XDG_CONFIG_HOME");
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
        QCOMPARE(model_->mode(), EntryModel::Mode::Standard);
    }

    void mode_setModeChangesMode() {
        model_->set_mode(EntryModel::Mode::Bookmarks);
        QCOMPARE(model_->mode(), EntryModel::Mode::Bookmarks);
        model_->set_mode(EntryModel::Mode::Standard);
        QCOMPARE(model_->mode(), EntryModel::Mode::Standard);
    }

    void mode_switchingDoesNotCrash() {
        model_->set_mode(EntryModel::Mode::Bookmarks);
        model_->set_mode(EntryModel::Mode::Dotfiles);
        model_->set_mode(EntryModel::Mode::Standard);
        QVERIFY(model_->rowCount() >= 0);
    }

    void mode_sameModeIsNoOp() {
        model_->set_mode(EntryModel::Mode::Standard);
        QCOMPARE(model_->rowCount(), 3);
    }

    void ssh_setModeChangesMode() {
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->mode(), EntryModel::Mode::SSH);
    }

    void ssh_missingConfigIsEmpty() {
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 0);
    }

    void ssh_malformedConfigIsEmpty() {
        write_file(ssh_config_path(), "{ this is not json");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 0);
    }

    void ssh_emptyArrayIsEmpty() {
        write_file(ssh_config_path(), R"({"ssh":[]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 0);
    }

    void ssh_loadsEntries() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com"},
                       {"name":"Staging","user":"bob","host":"staging.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 2);
    }

    void ssh_entryRolesArePopulated() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Prod"));
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 2).toString(),
                 QStringLiteral("alice@prod.example.com"));
    }

    void ssh_defaultIconWhenMissing() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 4).toString(),
                 QStringLiteral("network-server"));
    }

    void ssh_customIconIsKept() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com","icon":"utilities-terminal"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 4).toString(),
                 QStringLiteral("utilities-terminal"));
    }

    void ssh_skipsEntriesWithEmptyName() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"","user":"alice","host":"prod.example.com"},
                       {"user":"alice","host":"prod.example.com"},
                       {"name":"Good","user":"alice","host":"good.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Good"));
    }

    void ssh_entriesAreSortedCaseInsensitively() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"zulu","user":"u","host":"z.example.com"},
                       {"name":"Alpha","user":"u","host":"a.example.com"},
                       {"name":"mike","user":"u","host":"m.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 3);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Alpha"));
        QCOMPARE(model_->data(model_->index(1), Qt::UserRole + 1).toString(),
                 QStringLiteral("mike"));
        QCOMPARE(model_->data(model_->index(2), Qt::UserRole + 1).toString(),
                 QStringLiteral("zulu"));
    }

    void ssh_filterMatchesByName() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com"},
                       {"name":"Staging","user":"bob","host":"staging.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter(QStringLiteral("prod"));
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Prod"));
    }

    void ssh_filterNoMatchReturnsZero() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter(QStringLiteral("zzzzzzzz"));
        QCOMPARE(model_->rowCount(), 0);
    }

    void ssh_switchingAwayAndBackReloads() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 1);
        model_->set_mode(EntryModel::Mode::Standard);
        QCOMPARE(model_->rowCount(), 3);
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 1);
    }

    void ssh_launchInvalidRowReturnsFalse() {
        write_file(ssh_config_path(),
                   R"({"ssh":[
                       {"name":"Prod","user":"alice","host":"prod.example.com"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->launch(-1), false);
        QCOMPARE(model_->launch(model_->rowCount()), false);
    }

    void shellQuote_plainStringIsWrapped() {
        QCOMPARE(TerminalLauncher::shell_quote("hello"),
                 QStringLiteral("'hello'"));
    }

    void shellQuote_escapesSingleQuotes() {
        QCOMPARE(TerminalLauncher::shell_quote("a'b"),
                 QStringLiteral("'a'\\''b'"));
    }

    void shellQuote_emptyIsTwoQuotes() {
        QCOMPARE(TerminalLauncher::shell_quote(""),
                 QStringLiteral("''"));
    }

    void shellQuote_metacharsStayQuoted() {
        QCOMPARE(TerminalLauncher::shell_quote("a;b$c`d\"e\\f\ng"),
                 QStringLiteral("'a;b$c`d\"e\\f\ng'"));
    }

    void shellQuote_multipleQuotesAreEscaped() {
        QCOMPARE(TerminalLauncher::shell_quote("it's a 'test'"),
                 QStringLiteral("'it'\\''s a '\\''test'\\'''"));
    }

    void findTerminal_prefersConfigured() {
        QCOMPARE(TerminalLauncher::find_terminal("foot"),
                 QStringLiteral("foot"));
    }

    void findTerminal_configuredWinsOverEnv() {
        qputenv("TERMINAL", "kitty");
        QCOMPARE(TerminalLauncher::find_terminal("alacritty"),
                 QStringLiteral("alacritty"));
    }

    void findTerminal_fallsBackToEnv() {
        qputenv("TERMINAL", "wezterm");
        QCOMPARE(TerminalLauncher::find_terminal({}),
                 QStringLiteral("wezterm"));
    }

private:
    QString ssh_config_path() const {
        Q_ASSERT(temp_dir_ && temp_dir_->isValid());
        return temp_dir_->path() + "/ahoiplank/ssh.json";
    }

    void write_file(const QString& path, const QByteArray& content) {
        QVERIFY(temp_dir_ && temp_dir_->isValid());
        QVERIFY2(path.startsWith(temp_dir_->path()),
                 qPrintable(QStringLiteral("refusing to write outside temp dir: ") + path));
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(content);
        f.close();
    }

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
