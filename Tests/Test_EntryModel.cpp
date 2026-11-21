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

        const QString cache_dir = temp_dir_->path() + "/cache";
        QVERIFY(QDir().mkpath(cache_dir));
        QVERIFY(QDir().mkpath(temp_dir_->path() + "/.ssh"));

        qputenv("HOME", temp_dir_->path().toUtf8());
        qputenv("XDG_DATA_HOME", temp_dir_->path().toUtf8());
        qputenv("XDG_DATA_DIRS", temp_dir_->path().toUtf8());
        qputenv("XDG_CONFIG_HOME", temp_dir_->path().toUtf8());
        qputenv("XDG_CACHE_HOME", cache_dir.toUtf8());
    }

    void init() {
        qputenv("TERMINAL", "true");
        model_ = std::make_unique<EntryModel>();
    }

    void cleanup() {
        model_.reset();
        if (temp_dir_ && temp_dir_->isValid()) {
            QFile::remove(temp_dir_->path() + "/.ssh/config");
            QFile::remove(temp_dir_->path() + "/.ssh/known_hosts");
            QFile::remove(projects_config_path());
            QFile::remove(scripts_config_path());
            QFile::remove(frecency_path());
        }
        qunsetenv("TERMINAL");
    }

    void cleanupTestCase() {
        qunsetenv("HOME");
        qunsetenv("XDG_DATA_HOME");
        qunsetenv("XDG_DATA_DIRS");
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_CACHE_HOME");
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

    void cycle_mode_visitsAllModesIncludingScripts() {
        QSet<int> seen;
        const int n = static_cast<size_t>(EntryModel::Mode::ModesEnd);
        for (int i = 0; i < n; ++i) {
            seen.insert(static_cast<int>(model_->mode()));
            model_->cycle_mode(1);
        }
        QCOMPARE(seen.size(), n);
        QVERIFY(seen.contains(static_cast<int>(EntryModel::Mode::Scripts)));
        QCOMPARE(model_->mode(), EntryModel::Mode::Standard);
    }

    void cycle_mode_backwardsFromStandardReachesScripts() {
        model_->cycle_mode(-1);
        QCOMPARE(model_->mode(), EntryModel::Mode::Scripts);
    }

    void ssh_setModeChangesMode() {
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->mode(), EntryModel::Mode::SSH);
    }

    void ssh_missingFilesIsEmpty() {
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 0);
    }

    void ssh_emptyFilesIsEmpty() {
        write_file(temp_dir_->path() + "/.ssh/config", "");
        write_file(temp_dir_->path() + "/.ssh/known_hosts", "");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 0);
    }

    void ssh_loadsFromConfigAndKnownHosts() {
        write_file(temp_dir_->path() + "/.ssh/config",
                   "Host prod\n"
                   "    HostName prod.example.com\n"
                   "    User alice\n"
                   "\n"
                   "Host *\n"
                   "    ServerAliveInterval 60\n");

        write_file(temp_dir_->path() + "/.ssh/known_hosts",
                   "# comment line\n"
                   "|1|F3a9b8c7d6e5f4a3b2c1d0e9f8a=|9x8y7z6w5v4u3t2s1r0q9p8o7n6= ssh-ed25519 AAAAC3...\n"
                   "staging.example.com,10.0.0.5 ssh-ed25519 AAAAC3...\n");

        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);

        QCOMPARE(model_->rowCount(), 3);
    }

    void ssh_configEntryRolesArePopulated() {
        write_file(temp_dir_->path() + "/.ssh/config",
                   "Host prod\n"
                   "    HostName prod.example.com\n"
                   "    User alice\n");

        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("prod"));
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 2).toString(),
                 QStringLiteral("alice@prod"));
    }

    void ssh_knownHostBracketedPortIsFormatted() {
        const QString line = QChar('[') + QStringLiteral("192.168.1.50") + QChar(']')
                           + QStringLiteral(":2222 ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAI...\n");
        write_file(temp_dir_->path() + "/.ssh/known_hosts", line.toUtf8());

        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("192.168.1.50:2222"));
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 2).toString(),
                 QStringLiteral("192.168.1.50 -p 2222"));
    }

    void projects_setModeChangesMode() {
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->mode(), EntryModel::Mode::Projects);
    }

    void projects_missingConfigIsEmpty() {
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 0);
    }

    void projects_malformedConfigIsEmpty() {
        write_file(projects_config_path(), "{ this is not json");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 0);
    }

    void projects_emptyArrayIsEmpty() {
        write_file(projects_config_path(), R"({"projects":[]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 0);
    }

    void projects_emptyRootIsEmpty() {
        write_file(projects_config_path(), R"({})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 0);
    }

    void projects_loadsEntries() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/home/alex/Projekte/AhoiPlank"},
                       {"name":"kernel","path":"/home/alex/Projekte/kernel"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 2);
    }

    void projects_entryRolesArePopulated() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/home/alex/Projekte/AhoiPlank","comment":"launcher"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("ahoiplank"));
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 2).toString(),
                 QStringLiteral("/home/alex/Projekte/AhoiPlank"));
    }

    void projects_defaultIconWhenMissing() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/home/alex/Projekte/AhoiPlank"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 4).toString(),
                 QStringLiteral("gvim"));
    }

    void projects_customIconIsKept() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/home/alex/Projekte/AhoiPlank","icon":"folder-git"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 4).toString(),
                 QStringLiteral("folder-git"));
    }

    void projects_skipsEntriesWithEmptyName() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"","path":"/home/alex/Projekte/ghost"},
                       {"path":"/home/alex/Projekte/noname"},
                       {"name":"Good","path":"/home/alex/Projekte/good"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Good"));
    }

    void projects_skipsEntriesWithEmptyPath() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"NoPath","path":""},
                       {"name":"Good","path":"/home/alex/Projekte/good"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Good"));
    }

    void projects_entriesAreSortedCaseInsensitively() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"zulu","path":"/p/z"},
                       {"name":"Alpha","path":"/p/a"},
                       {"name":"mike","path":"/p/m"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 3);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Alpha"));
        QCOMPARE(model_->data(model_->index(1), Qt::UserRole + 1).toString(),
                 QStringLiteral("mike"));
        QCOMPARE(model_->data(model_->index(2), Qt::UserRole + 1).toString(),
                 QStringLiteral("zulu"));
    }

    void projects_filterMatchesByName() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/p/ahoiplank"},
                       {"name":"kernel","path":"/p/kernel"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        model_->set_filter(QStringLiteral("ahoi"));
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("ahoiplank"));
    }

    void projects_filterIsCaseInsensitive() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/p/ahoiplank"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        model_->set_filter(QStringLiteral("AHOI"));
        QCOMPARE(model_->rowCount(), 1);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("ahoiplank"));
    }

    void projects_switchingAwayAndBackReloads() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/p/ahoiplank"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 1);
        model_->set_mode(EntryModel::Mode::Standard);
        QCOMPARE(model_->rowCount(), 3);
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->rowCount(), 1);
    }

    void projects_launchInvalidRowReturnsFalse() {
        write_file(projects_config_path(),
                   R"({"projects":[
                       {"name":"ahoiplank","path":"/p/ahoiplank"}
                   ]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Projects);
        QCOMPARE(model_->launch(-1), false);
        QCOMPARE(model_->launch(model_->rowCount()), false);
    }

    void scripts_setModeChangesMode() {
        model_->set_mode(EntryModel::Mode::Scripts);
        QCOMPARE(model_->mode(), EntryModel::Mode::Scripts);
    }

    void scripts_missingFilesIsEmpty() {
        model_->set_mode(EntryModel::Mode::Scripts);
        QCOMPARE(model_->rowCount(), 0);
    }

    void scripts_malformedConfigIsEmpty() {
        write_file(scripts_config_path(), "{ this is not json");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Scripts);
        QCOMPARE(model_->rowCount(), 0);
    }

    void scripts_emptyArrayIsEmpty() {
        write_file(scripts_config_path(), R"({"scripts":[]})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Scripts);
        QCOMPARE(model_->rowCount(), 0);
    }

    void scripts_emptyRootIsEmpty() {
        write_file(scripts_config_path(), R"({})");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::Scripts);
        QCOMPARE(model_->rowCount(), 0);
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

    void frecency_emptyHistorySortsByName() {
        model_->set_filter("");
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Editor"));
        QCOMPARE(model_->data(model_->index(1), Qt::UserRole + 1).toString(),
                 QStringLiteral("Firefox"));
        QCOMPARE(model_->data(model_->index(2), Qt::UserRole + 1).toString(),
                 QStringLiteral("Terminal"));
    }

    void frecency_launchBumpsRank() {
        model_->set_mode(EntryModel::Mode::SSH);
        write_file(temp_dir_->path() + "/.ssh/config",
           "Host Alpha\n"
           "    HostName a.example.com\n"
           "Host Beta\n"
           "    HostName b.example.com\n"
           "Host Gamma\n"
           "    HostName c.example.com\n");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 3);

        model_->set_filter("");
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Alpha"));

        model_->launch(2);
        model_->set_filter("");
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Gamma"));
    }

    void frecency_persistsAcrossModelInstances() {
        model_->set_mode(EntryModel::Mode::SSH);
        write_file(temp_dir_->path() + "/.ssh/config",
           "Host Alpha\n"
           "    HostName a.example.com\n"
           "Host Beta\n"
           "    HostName b.example.com\n"
           "Host Gamma\n"
           "    HostName c.example.com\n");

        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");
        model_->launch(1);
        model_.reset();

        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Beta"));
    }

    void frecency_scopedPerMode() {
        write_file(temp_dir_->path() + "/.ssh/config",
           "Host Alpha\n"
           "    HostName a.example.com\n"
           "Host Beta\n"
           "    HostName b.example.com\n"
           "Host Gamma\n"
           "    HostName c.example.com\n");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");
        model_->launch(1);
        model_.reset();

        model_ = std::make_unique<EntryModel>();
        model_->set_filter("");
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Editor"));

        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Beta"));
    }

    void frecency_prunedOnMissingEntry() {
        write_file(ssh_config_path(),
                   "Host Alpha\n"
                   "    HostName a.example.com\n"
                   "    User u\n"
                   "Host Beta\n"
                   "    HostName b.example.com\n"
                   "    User u\n");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");
        model_->launch(1);
        model_.reset();

        write_file(ssh_config_path(),
                   "Host Alpha\n"
                   "    HostName a.example.com\n"
                   "    User u\n");

        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        QCOMPARE(model_->rowCount(), 1);

        write_file(ssh_config_path(),
                   "Host Alpha\n"
                   "    HostName a.example.com\n"
                   "    User u\n"
                   "Host Beta\n"
                   "    HostName b.example.com\n"
                   "    User u\n");

        model_.reset();
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Alpha"));
    }

    void frecency_cappedOnWrite() {
        write_file(ssh_config_path(),
                   "Host Alpha\n"
                   "    HostName a.example.com\n"
                   "    User u\n"
                   "Host Beta\n"
                   "    HostName b.example.com\n"
                   "    User u\n");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");

        for (int i = 0; i < 250; ++i) {
            model_->launch(1);
        }
        model_.reset();

        QFile f(frecency_path());
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray content = f.readAll();
        f.close();

        const QJsonObject root = QJsonDocument::fromJson(content).object();
        const QJsonObject modes = root.value("modes").toObject();
        const QJsonObject ssh = modes.value("ssh").toObject();
        const QJsonObject beta = ssh.value("u@Beta").toObject();

        QVERIFY(beta.value("count").toInt() <= 200);
    }

    void frecency_tiebreakInFilteredResults() {
        write_file(ssh_config_path(),
                   "Host Prod-A\n"
                   "    HostName a.example.com\n"
                   "    User u\n"
                   "Host Prod-B\n"
                   "    HostName b.example.com\n"
                   "    User u\n");
        model_ = std::make_unique<EntryModel>();
        model_->set_mode(EntryModel::Mode::SSH);
        model_->set_filter("");

        model_->launch(1);
        model_->set_filter(QStringLiteral("prod"));

        QCOMPARE(model_->rowCount(), 2);
        QCOMPARE(model_->data(model_->index(0), Qt::UserRole + 1).toString(),
                 QStringLiteral("Prod-B"));
    }

private:
    QString ssh_config_path() const {
        Q_ASSERT(temp_dir_ && temp_dir_->isValid());
        return temp_dir_->path() + "/.ssh/config";
    }

    QString projects_config_path() const {
        Q_ASSERT(temp_dir_ && temp_dir_->isValid());
        return temp_dir_->path() + "/ahoiplank/projects.json";
    }

    QString scripts_config_path() const {
        Q_ASSERT(temp_dir_ && temp_dir_->isValid());
        return temp_dir_->path() + "/ahoiplank/scripts.json";
    }

    QString frecency_path() const {
        Q_ASSERT(temp_dir_ && temp_dir_->isValid());
        return temp_dir_->path() + "/cache/ahoiplank/frecency.json";
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
