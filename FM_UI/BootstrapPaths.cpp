#include"BootstrapPaths.h"

#include<QCoreApplication>
#include<QDir>
#include<QFileInfo>
#include<QStandardPaths>

#include<stdexcept>
#include<string>

namespace {
    QString requireExistingFile(const QString& path, const char* description) {
        const QString cleanPath = QDir::cleanPath(path);
        if (!QFileInfo::exists(cleanPath)) {
            throw std::runtime_error(std::string(description) + " not found at " + cleanPath.toStdString());
        }
        return cleanPath;
    }

    QString makeRuntimeDatabaseDirectory() {
        const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataPath.isEmpty()) {
            throw std::runtime_error("Qt did not provide an application data location for the runtime database");
        }

        const QString databaseDirectory = QDir::cleanPath(appDataPath + QStringLiteral("/database"));
        if (!QDir().mkpath(databaseDirectory)) {
            throw std::runtime_error("failed to create runtime database directory at " + databaseDirectory.toStdString());
        }

        return databaseDirectory;
    }
}

namespace BootstrapPaths {
    GameBootstrapOptions createGameBootstrapOptions() {
        const QString executableDatabaseDirectory =
            QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/database"));
        const QString schemaPath = requireExistingFile(
            executableDatabaseDirectory + QStringLiteral("/schema.sql"),
            "SQLite schema asset");
        const QString seedPath = requireExistingFile(
            executableDatabaseDirectory + QStringLiteral("/seed.sql"),
            "SQLite seed asset");

        const QString runtimeDatabaseDirectory = makeRuntimeDatabaseDirectory();
        const QString runtimeDatabasePath =
            QDir::cleanPath(runtimeDatabaseDirectory + QStringLiteral("/superlig_runtime.db"));

        GameBootstrapOptions options;
        options.mode = GameBootstrapMode::Sqlite;
        options.sqliteDbPath = runtimeDatabasePath.toStdString();
        options.sqliteSchemaPath = schemaPath.toStdString();
        options.sqliteSeedPath = seedPath.toStdString();
        options.initializeDatabaseWithSeed = true;
        return options;
    }
}
