#include <QGuiApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <cstdlib>
#include <exception>

#include "BootstrapPaths.h"
#include "GameFacade.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FootballManager"));
    QCoreApplication::setApplicationName(QStringLiteral("FootballManager"));

    GameBootstrapOptions bootstrapOptions;
    try {
        bootstrapOptions = BootstrapPaths::createGameBootstrapOptions();
    }
    catch (const std::exception& ex) {
        qCritical() << "Failed to resolve game bootstrap paths:" << ex.what();
        return EXIT_FAILURE;
    }

    QQmlApplicationEngine engine;
    GameFacade facade(bootstrapOptions);
    engine.rootContext()->setContextProperty("gameFacade", &facade);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("FM_UI", "Main");

    return app.exec();
}
