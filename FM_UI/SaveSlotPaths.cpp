#include"SaveSlotPaths.h"

#include"BootstrapPaths.h"

#include<QDir>
#include<QStandardPaths>

#include<stdexcept>
#include<string>

namespace {
    QString requireDirectory(const QString& path, const char* description) {
        const QString cleanPath = QDir::cleanPath(path);
        if (!QDir().mkpath(cleanPath)) {
            throw std::runtime_error(std::string("failed to create ") + description + " at " + cleanPath.toStdString());
        }
        return cleanPath;
    }

    void requireSaveSlotId(const QString& saveSlotId) {
        if (saveSlotId.trimmed().isEmpty()) {
            throw std::invalid_argument("save slot id cannot be empty");
        }
        if (saveSlotId.contains(QLatin1Char('/')) || saveSlotId.contains(QLatin1Char('\\'))) {
            throw std::invalid_argument("save slot id cannot contain path separators");
        }
    }
}

namespace SaveSlotPaths {
    QString appDataRoot() {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (path.isEmpty()) {
            throw std::runtime_error("Qt did not provide an application data location for save slots");
        }
        return QDir::cleanPath(path);
    }

    QString savesRootDirectory() {
        return requireDirectory(appDataRoot() + QStringLiteral("/saves"), "save root directory");
    }

    QString defaultSaveSlotId() {
        return QStringLiteral("default");
    }

    QString defaultSaveNameForSlot(const QString& saveSlotId) {
        if (saveSlotId == defaultSaveSlotId()) {
            return QStringLiteral("Default Save");
        }
        return saveSlotId;
    }

    QString saveSlotDirectory(const QString& saveSlotId) {
        requireSaveSlotId(saveSlotId);
        return requireDirectory(savesRootDirectory() + QStringLiteral("/") + saveSlotId, "save slot directory");
    }

    QString saveDatabasePath(const QString& saveSlotId) {
        return QDir::cleanPath(saveSlotDirectory(saveSlotId) + QStringLiteral("/game.db"));
    }

    GameBootstrapOptions createBootstrapOptionsForSaveSlot(
        const QString& saveSlotId,
        DatabaseOpenMode openMode) {
        GameBootstrapOptions options;
        options.mode = GameBootstrapMode::Sqlite;
        options.databaseOpenMode = openMode;
        options.sqliteDbPath = saveDatabasePath(saveSlotId).toStdString();
        options.sqliteSchemaPath = BootstrapPaths::schemaAssetPath().toStdString();
        options.sqliteSeedPath = BootstrapPaths::seedAssetPath().toStdString();
        options.saveSlotId = saveSlotId.toStdString();
        options.saveName = defaultSaveNameForSlot(saveSlotId).toStdString();
        return options;
    }

    GameBootstrapOptions createDefaultSaveBootstrapOptions() {
        return createBootstrapOptionsForSaveSlot(
            defaultSaveSlotId(),
            DatabaseOpenMode::CreateFromSeedIfMissing);
    }
}
