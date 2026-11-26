#include "BookmarksSource.h"
#include "version.h"
#include "EntryModel.h"
#include "ProjectsSource.h"
#include "IconProvider.h"
#include "GeneralConfig.h"
#include <QFile>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QObject>
#include <QString>
#include <QWindow>
#include <QScreen>
#include <QTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>
#include <QQuickWindow>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QIcon>
#include <LayerShellQt/Window>
#include <LayerShellQt/Shell>
#include <print>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

extern const char* embedded_version;
static const char* k_socket_name = "ahoiplank-socket";

static bool try_send_command(const char* cmd) {
    QLocalSocket sock;
    sock.connectToServer(k_socket_name);
    if (!sock.waitForConnected(500)) {
        return false;
    }
    sock.write(cmd);
    if (!sock.waitForBytesWritten(500)) {
        return false;
    }
    sock.disconnectFromServer();
    if (sock.state() != QLocalSocket::UnconnectedState) {
        sock.waitForDisconnected(200);
    }
    return true;
}

static bool is_wsl() {
    return std::getenv("WSL_DISTRO_NAME") != nullptr ||
        std::getenv("WSL_INTEROP") != nullptr;
}

static bool is_ssh() {
    return std::getenv("SSH_CONNECTION") != nullptr ||
        std::getenv("SSH_CLIENT") != nullptr ||
        std::getenv("SSH_TTY") != nullptr;
}

int main(int argc, char* argv[]) {
    bool daemon_start = false;
    bool daemon_stop = false;
    bool cold_show = false;
    EntryModel::Mode mode = EntryModel::Mode::Standard;
    if (argc > 1) {
        const QString arg = QString::fromLocal8Bit(argv[1]);
        if (arg == "--version") {
            std::println("{}", embedded_version);
            return 0;
        }
        else if (arg == "--help") {
            std::println("ahoiplank is a customizable launcher for Wayland.\n"
                    "You can find more at https://github.com/martuscellifaria/ahoiplank\n\n"
                    "Usage: ahoiplank [OPTIONAL], "
                    "where [OPTIONAL] is one among the following:\n\n"
                    "    --daemon-start\n"
                    "\tStarts ahoiplank daemon in background to better user feel.\n"
                    "    --daemon-stop\n"
                    "\tStops ahoiplank daemon.\n"
                    "    --bookmarks\n"
                    "\tOpens the launcher in bookmarks mode.\n"
                    "    --dotfiles\n"
                    "\tOpens the launcher in dotfiles mode.\n"
                    "    --ssh\n"
                    "\tOpens the launcher in ssh mode.\n"
                    "    --projects\n"
                    "\tOpens the launcher in projects mode.\n"
                    "    --scripts\n"
                    "\tOpens the launcher in scripts mode.\n"
                    "    --pin-project\n"
                    "\tPins project to be displayed at projects mode.\n"
                    "    --unpin-project <project-name>\n"
                    "\tUnpins project from the projects mode.\n"
                    "    --list-projects\n"
                    "\tLists project from the projects mode.\n"
                    "    --version\n"
                    "\tShows version metadata.\n"
                    "    --help\n"
                    "\tShows this helper.");
            return 0;
        }
        else if (arg == "--daemon-stop") {
            daemon_stop = true;
        }
        else if (arg == "--daemon-start") {
            daemon_start = true;
        }
        else if (arg == "--bookmarks") {
            mode = EntryModel::Mode::Bookmarks;
        }
        else if (arg == "--dotfiles") {
            mode = EntryModel::Mode::Dotfiles;
        }
        else if (arg == "--ssh") {
            mode = EntryModel::Mode::SSH;
        }
        else if (arg == "--projects") {
            mode = EntryModel::Mode::Projects;
        }
        else if (arg == "--scripts") {
            mode = EntryModel::Mode::Scripts;
        }
        else if (arg == "--pin-project") {
            ProjectsSource::pin_project();
            return 0;
        }
        else if (arg == "--unpin-project") {
            if (argc > 2) {
                const QString project_name = QString::fromLocal8Bit(argv[2]);
                ProjectsSource::unpin_project(project_name);
                return 0;
            }
            ProjectsSource::unpin_project();
            return 0;
        }
        else if (arg == "--list-projects") {
            ProjectsSource::list_projects();
            return 0;
        }
    }

    if (is_wsl()) {
        std::println(stderr, "ahoiplank: running on WSL "
                "may mess things up. Not starting.");
        return 1;
    }

    if (is_ssh()) {
        std::println(stderr, "ahoiplank: running on an SSH "
                "session is not the way intended. Not starting");
        return 1;
    }

    if (daemon_stop) {
        try_send_command("quit");
        return 0;
    }

    if (daemon_start) {
        if (try_send_command("ping")) {
            return 0;
        }
    }
    else {
        const QByteArray cmd = "toggle:" + EntryModel::mode_to_string(mode).toUtf8();
        if (try_send_command(cmd.constData())) {
            return 0;
        }
        cold_show = true;
    }

    const QString wl_display = qEnvironmentVariable("WAYLAND_DISPLAY");
    const QString runtime_dir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    const QString session_type = qEnvironmentVariable("XDG_SESSION_TYPE");

    if (wl_display.isEmpty() || runtime_dir.isEmpty()) {
        std::println(stderr, "ahoiplank: WAYLAND_DISPLAY "
                "or XDG_RUNTIME_DIR not set.");
        return 1;
    }

    if (session_type.isEmpty() || session_type != "wayland") {
        std::println(stderr, "ahoiplank: XDG_SESSION_TYPE "
                "not set to wayland.");
        return 1;
    }

    const QString wl_socket = QDir(runtime_dir).filePath(wl_display);
    if (!QFile::exists(wl_socket)) {
        std::println(stderr, "ahoiplank: no Wayland socket at {}. "
                "Please make sure you are on Wayland.", wl_socket.toStdString());
        return 1;
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        std::println(stderr, "ahoiplank: fork failed.");
        return 1;
    }
    if (pid > 0) {
        ::_exit(0);
    }

    ::setsid();

    const int devnull = ::open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        ::dup2(devnull, STDIN_FILENO);
        ::dup2(devnull, STDOUT_FILENO);
        ::dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) {
            ::close(devnull);
        }
    }

    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QIcon::setThemeName(QStringLiteral("Adwaita"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/plank.png")));
    app.setDesktopFileName(QStringLiteral("ahoiplank"));

    QLocalServer::removeServer(k_socket_name);
    QLocalServer server;
    if (!server.listen(k_socket_name)) {
        qWarning() << "could not listen on" << k_socket_name << ":" << server.errorString();
        return -1;
    }

    EntryModel controller;
    GeneralConfig general_config;

    QQmlApplicationEngine engine;
    engine.addImageProvider("icon", new IconProvider);
    engine.rootContext()->setContextProperty("app", &controller);
    engine.rootContext()->setContextProperty("theme", &general_config);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("ahoiplankQml", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QWindow* win = qobject_cast<QWindow*>(engine.rootObjects().constFirst());
    if (!win) {
        return -1;
    }

    LayerShellQt::Window* layer = LayerShellQt::Window::get(win);
    if (layer) {
        layer->setScope("ahoiplank");
        layer->setLayer(LayerShellQt::Window::LayerOverlay);
        layer->setKeyboardInteractivity(
            LayerShellQt::Window::KeyboardInteractivityExclusive);
        layer->setAnchors({ LayerShellQt::Window::AnchorTop });
        layer->setExclusiveZone(-1);
        layer->setWantsToBeOnActiveScreen(true);
    }

    auto apply_margins = [&]() {
        if (!layer) {
            return;
        }
        const QScreen* scr = win->screen();
        if (!scr) {
            scr = QGuiApplication::primaryScreen();
        }
        const int screen_h = scr ? scr->geometry().height() : 0;
        const int win_h = general_config.screen_height();

        const QString pos = general_config.vertical_position();
        bool is_number = false;
        const int as_int = pos.toInt(&is_number);

        int margin_top = 0;
        if (is_number) {
            margin_top = qMax(0, as_int);
        }
        else if (pos == "top") {
            margin_top = 0;
        }
        else if (pos == "bottom") {
            margin_top = qMax(0, screen_h - win_h);
        }
        else {
            margin_top = qMax(0, (screen_h - win_h) / 2);
        }

        layer->setMargins(QMargins(0, margin_top, 0, 0));
    };

    GeneralConfig::generate_config_dotfile();
    BookmarksSource::generate_bookmarks_dotfile();

    QTimer::singleShot(0, win, apply_margins);

    QFileSystemWatcher config_watcher;
    const QString config_dir =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + "/ahoiplank";
    const QString config_file = config_dir + "/config.json";

    config_watcher.addPath(config_dir);
    if (QFile::exists(config_file)) {
        config_watcher.addPath(config_file);
    }

    auto reload_config = [&]() {
        if (!QFile::exists(config_file)) {
            return;
        }
        if (!config_watcher.files().contains(config_file)) {
            config_watcher.addPath(config_file);
        }
        general_config.reload();
    };

    QObject::connect(&config_watcher, &QFileSystemWatcher::fileChanged,
                     &general_config, reload_config);

    QObject::connect(&config_watcher, &QFileSystemWatcher::directoryChanged,
                     &general_config, reload_config);

    auto toggle = [&]() {
        const bool show = !win->isVisible();
        if (show) {
            GeneralConfig::generate_config_dotfile();
            BookmarksSource::generate_bookmarks_dotfile();
            controller.set_text_editor(general_config.text_editor());
            controller.set_terminal(general_config.terminal());
            apply_margins();
            controller.request_reset();
            controller.set_filter(QStringLiteral(""));
            win->show();
            win->requestActivate();
        }
        else {
            win->hide();
        }
    };
    auto quit = [&]() { QCoreApplication::quit(); };

    QObject::connect(&controller, &EntryModel::dismissed, &app, [&]() {
        win->hide();
    });

    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        while (auto* c = server.nextPendingConnection()) {
            QTimer::singleShot(2000, c, [c]() {
                if (c->state() != QLocalSocket::UnconnectedState) {
                    c->abort();
                }
            });
            QObject::connect(c, &QLocalSocket::readyRead, c, [c, &toggle, &quit, &controller]() {
                const QByteArray msg = c->readAll().trimmed();
                if (msg.isEmpty()) {
                    return;
                }
                if (msg.startsWith("toggle")) {
                EntryModel::Mode m = EntryModel::Mode::Standard;
                    if (msg.size() > 6 && msg.at(6) == ':') {
                        m = EntryModel::mode_from_string(QString::fromUtf8(msg.mid(7)));
                    }
                    controller.set_mode(m);
                    toggle();
                }
                else if (msg == "ping") {
                }
                else if (msg == "quit") {
                    quit();
                }
                c->disconnectFromServer();
            });
            QObject::connect(c, &QLocalSocket::disconnected,
                             c, &QLocalSocket::deleteLater);
        }
    });

    if (cold_show) {
        controller.set_mode(mode);
        controller.set_text_editor(general_config.text_editor());
        controller.set_terminal(general_config.terminal());
        controller.set_filter(QStringLiteral(""));
        controller.request_reset();
        win->show();
        win->requestActivate();
    }

    return app.exec();
}
