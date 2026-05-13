#include <QGuiApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <cstdlib>

#include "GameFacade.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("FootballManager"));
    QCoreApplication::setApplicationName(QStringLiteral("FootballManager"));

    QQmlApplicationEngine engine;
    GameFacade facade;
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
