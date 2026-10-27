#include "version.h"
#include "entry_model.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QObject>
#include <QString>
#include <QWindow>
#include <QTimer>
#include <LayerShellQt/Window>
#include <LayerShellQt/Shell>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/file.h>
    #include <unistd.h>
    #include <cerrno>
#endif

extern const char* embedded_version;


int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    entry_model controller;

    QQmlApplicationEngine engine;
    engine.addImageProvider("icon", new icon_provider);
    engine.rootContext()->setContextProperty("app", &controller);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("AhoiPlankQml", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QWindow* win = qobject_cast<QWindow*>(engine.rootObjects().constFirst());
    if (win) {
        if (auto* layer = LayerShellQt::Window::get(win)) {
            layer->setLayer(LayerShellQt::Window::LayerOverlay);
            layer->setKeyboardInteractivity(
                LayerShellQt::Window::KeyboardInteractivityExclusive);
	    layer->setAnchors({ LayerShellQt::Window::AnchorTop });
	    layer->setMargins(QMargins(0, 120, 0, 0));
            layer->setExclusiveZone(-1);
	    layer->setWantsToBeOnActiveScreen(true);
        }
    }
    win->show();

    return app.exec();
}
