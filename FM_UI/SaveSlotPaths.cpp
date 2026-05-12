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

    bool hasWindowsProblematicCharacter(const QString& value) {
        static const QString problematic = QStringLiteral("<>:\"|?*");
        for (const QChar character : value) {
            if (problematic.contains(character)) {
                return true;
            }
        }
        return false;
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

    QString ensureSavesRootDirectory() {
        return requireDirectory(appDataRoot() + QStringLiteral("/saves"), "save root directory");
    }

    QString savesRootDirectoryPath() {
        return QDir::cleanPath(appDataRoot() + QStringLiteral("/saves"));
    }

    QString defaultSaveSlotId() {
        return QStringLiteral("default");
    }

    bool isValidSaveSlotId(const QString& saveSlotId) {
        const QString trimmed = saveSlotId.trimmed();
        if (trimmed.isEmpty() || trimmed != saveSlotId) {
            return false;
        }
        if (trimmed == QStringLiteral(".") || trimmed == QStringLiteral("..")) {
            return false;
        }
        if (trimmed.contains(QLatin1Char('/')) || trimmed.contains(QLatin1Char('\\'))) {
            return false;
        }
        return !hasWindowsProblematicCharacter(trimmed);
    }

    QString requireValidSaveSlotId(const QString& saveSlotId) {
        if (!isValidSaveSlotId(saveSlotId)) {
            throw std::invalid_argument("save slot id must be non-empty and cannot contain path separators or reserved filename characters");
        }
        return saveSlotId;
    }

    QString defaultSaveNameForSlot(const QString& saveSlotId) {
        if (saveSlotId == defaultSaveSlotId()) {
            return QStringLiteral("Default Save");
        }
        return saveSlotId;
    }

    QString saveSlotDirectoryPath(const QString& saveSlotId) {
        return QDir::cleanPath(savesRootDirectoryPath() + QStringLiteral("/") + requireValidSaveSlotId(saveSlotId));
    }

    QString saveDatabasePath(const QString& saveSlotId) {
        return QDir::cleanPath(saveSlotDirectoryPath(saveSlotId) + QStringLiteral("/game.db"));
    }

    QString ensureSaveSlotDirectory(const QString& saveSlotId) {
        return requireDirectory(
            ensureSavesRootDirectory() + QStringLiteral("/") + requireValidSaveSlotId(saveSlotId),
            "save slot directory");
    }

    QString ensureSaveDatabasePath(const QString& saveSlotId) {
        return QDir::cleanPath(ensureSaveSlotDirectory(saveSlotId) + QStringLiteral("/game.db"));
    }

    GameBootstrapOptions createBootstrapOptionsForSaveSlot(
        const QString& saveSlotId,
        DatabaseOpenMode openMode) {
        GameBootstrapOptions options;
        options.mode = GameBootstrapMode::Sqlite;
        options.databaseOpenMode = openMode;
        if (openMode == DatabaseOpenMode::OpenExisting) {
            options.sqliteDbPath = saveDatabasePath(saveSlotId).toStdString();
        } else {
            options.sqliteSchemaPath = BootstrapPaths::schemaAssetPath().toStdString();
            options.sqliteSeedPath = BootstrapPaths::seedAssetPath().toStdString();
            options.sqliteDbPath = ensureSaveDatabasePath(saveSlotId).toStdString();
        }
        const QString validSaveSlotId = requireValidSaveSlotId(saveSlotId);
        options.saveSlotId = validSaveSlotId.toStdString();
        options.saveName = defaultSaveNameForSlot(validSaveSlotId).toStdString();
        return options;
    }

    GameBootstrapOptions createDefaultSaveBootstrapOptions() {
        return createBootstrapOptionsForSaveSlot(
            defaultSaveSlotId(),
            DatabaseOpenMode::CreateFromSeedIfMissing);
    }
}
