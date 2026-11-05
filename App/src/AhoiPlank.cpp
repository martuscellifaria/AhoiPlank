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
#include <LayerShellQt/Window>
#include <LayerShellQt/Shell>
#include <print>
#include <sys/file.h>
#include <unistd.h>

extern const char* embedded_version;


static const char* kSocketName = "ahoiplank-socket";

static bool try_send_command(const char* cmd) {
    QLocalSocket sock;
    sock.connectToServer(kSocketName);
    if (!sock.waitForConnected(500)) {
        return false;
    }
    sock.write(cmd);
    sock.waitForBytesWritten(500);
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

    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QQuickStyle::setStyle("Basic");

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
        if (try_send_command("toggle")) {
            return 0;
        }
        cold_show = true;
    }

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

    if (auto* layer = LayerShellQt::Window::get(win)) {
        layer->setScope("AhoiPlank");
        layer->setLayer(LayerShellQt::Window::LayerOverlay);
        layer->setKeyboardInteractivity(
            LayerShellQt::Window::KeyboardInteractivityExclusive);
        layer->setAnchors({ LayerShellQt::Window::AnchorTop });
        layer->setExclusiveZone(-1);
        layer->setWantsToBeOnActiveScreen(true);

        QTimer::singleShot(0, win, [win, layer, &theme_config]() {
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
        });
    }

    auto toggle = [&]() {
        const bool show = !win->isVisible();
        if (show) {
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
