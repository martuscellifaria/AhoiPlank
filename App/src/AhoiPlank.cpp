#include "version.h"
#include "EntryModel.h"
#include "ThemeConfig.h"
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
#include <QQuickStyle>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <LayerShellQt/Window>
#include <LayerShellQt/Shell>
#include <print>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

extern const char* embedded_version;


static const char* kSocketName = "ahoiplank-socket";

static QString socket_path() {
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return QDir(runtime).filePath(kSocketName);
}

static bool try_send_raw(const char* cmd) {
    const QString path = socket_path();
    if (path.isEmpty()) {
        return false;
    }

    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    const QByteArray path_bytes = path.toLocal8Bit();
    if (path_bytes.size() + 1 > static_cast<int>(sizeof(addr.sun_path))) {
        ::close(fd);
        return false;
    }
    std::strncpy(addr.sun_path, path_bytes.constData(), sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return false;
    }

    const size_t len = std::strlen(cmd);
    if (::write(fd, cmd, len) != static_cast<ssize_t>(len)) {
        ::close(fd);
        return false;
    }

    ::close(fd);
    return true;
}


int main(int argc, char* argv[]) {
    bool daemon_start = false;
    bool daemon_stop = false;
    bool cold_show = false;

    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--version") {
            std::println("{}", embedded_version);
            return 0;
        }
        else if (arg == "--daemon-start") {
            daemon_start = true;
        }
        else if (arg == "--daemon-stop") {
            daemon_stop = true;
        }
    }

    if (daemon_stop) {
        try_send_raw("quit");
        return 0;
    }

    if (daemon_start) {
        if (try_send_raw("ping")) {
            return 0;
        }
    }
    else {
        if (try_send_raw("toggle")) {
            return 0;
        }
        cold_show = true;
    }

    if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        std::println(stderr, "AhoiPlank: no Wayland display (WAYLAND_DISPLAY is not set)");
        return 1;
    }

    const QString wl_socket = QDir(qEnvironmentVariable("XDG_RUNTIME_DIR"))
                                  .filePath(qEnvironmentVariable("WAYLAND_DISPLAY"));
    if (!QFile::exists(wl_socket)) {
        std::println(stderr, "AhoiPlank: WAYLAND_DISPLAY={} but {} does not exist",
                     qEnvironmentVariable("WAYLAND_DISPLAY").toStdString(),
                     wl_socket.toStdString());
        return 1;
    }

    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QQuickStyle::setStyle("Basic");

    QLocalServer::removeServer(kSocketName);
    QLocalServer server;
    if (!server.listen(kSocketName)) {
        qWarning() << "could not listen on" << kSocketName << ":" << server.errorString();
        return -1;
    }

    EntryModel controller;
    ThemeConfig theme_config;

    QQmlApplicationEngine engine;
    engine.addImageProvider("icon", new IconProvider);
    engine.rootContext()->setContextProperty("app", &controller);
    engine.rootContext()->setContextProperty("theme", &theme_config);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("AhoiPlankQml", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QWindow* win = qobject_cast<QWindow*>(engine.rootObjects().constFirst());
    if (!win) {
        return -1;
    }

    LayerShellQt::Window* layer = LayerShellQt::Window::get(win);
    if (layer) {
        layer->setScope("AhoiPlank");
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
        const int win_h = theme_config.screen_height();

        const QString pos = theme_config.vertical_position();
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

    QTimer::singleShot(0, win, apply_margins);

    auto toggle = [&]() {
        const bool show = !win->isVisible();
        if (show) {
            theme_config.reload();
            apply_margins();
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
            QObject::connect(c, &QLocalSocket::readyRead, [c, &toggle, &quit]() {
                const QByteArray msg = c->readAll().trimmed();
                if (msg == "toggle") {
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
        win->show();
        win->requestActivate();
    }

    return app.exec();
}
