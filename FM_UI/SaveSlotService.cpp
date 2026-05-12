#include"SaveSlotService.h"

#include"SaveSlotPaths.h"

#include"fm/data/SaveMetadata.h"
#include"fm/data/SqliteSaveMetadataRepository.h"

#include<QDateTime>
#include<QDebug>
#include<QDir>
#include<QFileInfo>
#include<QRegularExpression>
#include<QSettings>

#include<algorithm>
#include<exception>
#include<stdexcept>

namespace {
    constexpr const char* LastSaveSlotSettingsKey = "saveSlots/lastSaveSlotId";

    QString fromStd(const std::string& value) {
        return QString::fromStdString(value);
    }

    SaveSlotInfo toSaveSlotInfo(const SaveMetadata& metadata) {
        SaveSlotInfo info;
        info.saveSlotId = fromStd(metadata.saveSlotId);
        info.saveName = fromStd(metadata.saveName);
        info.managerName = fromStd(metadata.managerName);
        info.managedLeagueId = static_cast<int>(metadata.managedLeagueId);
        info.managedTeamId = static_cast<int>(metadata.managedTeamId);
        info.currentDate = fromStd(metadata.currentDate);
        info.createdAtUtc = fromStd(metadata.createdAtUtc);
        info.updatedAtUtc = fromStd(metadata.updatedAtUtc);
        info.schemaVersion = metadata.schemaVersion;
        info.worldVersion = metadata.worldVersion;
        info.isValid = true;
        return info;
    }

    QString sanitizedSlotBase(QString text) {
        text = text.trimmed().toLower();
        text.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")), QStringLiteral("_"));
        text.replace(QRegularExpression(QStringLiteral("_+")), QStringLiteral("_"));
        text = text.trimmed();
        while (text.startsWith(QLatin1Char('_')) || text.startsWith(QLatin1Char('-'))) {
            text.remove(0, 1);
        }
        while (text.endsWith(QLatin1Char('_')) || text.endsWith(QLatin1Char('-'))) {
            text.chop(1);
        }
        if (text.isEmpty() || text == QStringLiteral(".") || text == QStringLiteral("..")) {
            return QStringLiteral("save");
        }
        return text.left(48);
    }

    QString slotDirectoryCanonicalPath(const QString& saveSlotId) {
        return QFileInfo(SaveSlotPaths::saveSlotDirectoryPath(saveSlotId)).canonicalFilePath();
    }
}

QList<SaveSlotInfo> SaveSlotService::listSaveSlots() const {
    QList<SaveSlotInfo> saveSlots;

    const QDir savesRoot(SaveSlotPaths::savesRootDirectoryPath());
    if (!savesRoot.exists()) {
        return saveSlots;
    }

    const QFileInfoList entries = savesRoot.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& entry : entries) {
        const QString saveSlotId = entry.fileName();
        if (!SaveSlotPaths::isValidSaveSlotId(saveSlotId)) {
            qWarning() << "[SaveSlotService] Skipping invalid save slot directory name:" << saveSlotId;
            continue;
        }

        const QString dbPath = SaveSlotPaths::saveDatabasePath(saveSlotId);
        if (!QFileInfo::exists(dbPath)) {
            continue;
        }

        try {
            SqliteSaveMetadataRepository repository(
                dbPath.toStdString(),
                SaveMetadataRepositoryMode::ReadExisting);
            if (!repository.exists()) {
                qWarning() << "[SaveSlotService] Skipping save without metadata:" << saveSlotId;
                continue;
            }
            SaveSlotInfo info = toSaveSlotInfo(repository.load());
            if (info.saveSlotId.isEmpty()) {
                info.saveSlotId = saveSlotId;
            }
            saveSlots.push_back(info);
        } catch (const std::exception& ex) {
            qWarning() << "[SaveSlotService] Skipping unreadable save slot" << saveSlotId << ":" << ex.what();
        }
    }

    std::sort(saveSlots.begin(), saveSlots.end(), [](const SaveSlotInfo& lhs, const SaveSlotInfo& rhs) {
        return lhs.updatedAtUtc > rhs.updatedAtUtc;
    });
    return saveSlots;
}

QString SaveSlotService::createUniqueSaveSlotId(const QString& saveNameOrManagerName) const {
    const QString base = sanitizedSlotBase(saveNameOrManagerName);
    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    QString candidate = QStringLiteral("%1_%2").arg(base, timestamp);
    int suffix = 2;
    while (saveSlotExists(candidate)) {
        candidate = QStringLiteral("%1_%2_%3").arg(base, timestamp).arg(suffix++);
    }
    return candidate;
}

GameBootstrapOptions SaveSlotService::createNewSaveBootstrapOptions(
    const QString& saveSlotId,
    const QString& saveName) const {
    GameBootstrapOptions options = SaveSlotPaths::createBootstrapOptionsForSaveSlot(
        saveSlotId,
        DatabaseOpenMode::ResetFromSeed);
    options.saveName = saveName.trimmed().isEmpty()
        ? std::string("New Save")
        : saveName.trimmed().toStdString();
    return options;
}

GameBootstrapOptions SaveSlotService::loadExistingSaveBootstrapOptions(const QString& saveSlotId) const {
    if (!saveSlotExists(saveSlotId)) {
        throw std::invalid_argument("save slot database does not exist: " + saveSlotId.toStdString());
    }
    return SaveSlotPaths::createBootstrapOptionsForSaveSlot(saveSlotId, DatabaseOpenMode::OpenExisting);
}

bool SaveSlotService::saveSlotExists(const QString& saveSlotId) const {
    if (!SaveSlotPaths::isValidSaveSlotId(saveSlotId)) {
        return false;
    }
    return QFileInfo::exists(SaveSlotPaths::saveDatabasePath(saveSlotId));
}

bool SaveSlotService::deleteSaveSlot(const QString& saveSlotId, QString* errorMessage) const {
    if (errorMessage) {
        errorMessage->clear();
    }

    if (!SaveSlotPaths::isValidSaveSlotId(saveSlotId)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid save slot id.");
        }
        return false;
    }

    const QDir savesRoot(SaveSlotPaths::savesRootDirectoryPath());
    if (!savesRoot.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Save slot does not exist.");
        }
        return false;
    }

    const QString rootCanonical = QDir::cleanPath(QFileInfo(savesRoot.absolutePath()).canonicalFilePath());
    const QString slotCanonical = QDir::cleanPath(slotDirectoryCanonicalPath(saveSlotId));
    const QString rootPrefix = rootCanonical.endsWith(QLatin1Char('/'))
        ? rootCanonical
        : rootCanonical + QLatin1Char('/');
    if (rootCanonical.isEmpty()
        || slotCanonical.isEmpty()
        || !slotCanonical.startsWith(rootPrefix, Qt::CaseInsensitive)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Refusing to delete a path outside the saves directory.");
        }
        return false;
    }

    QDir slotDirectory(slotCanonical);
    if (!slotDirectory.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Save slot does not exist.");
        }
        return false;
    }

    if (!slotDirectory.removeRecursively()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to delete save slot.");
        }
        return false;
    }

    if (lastSaveSlotId() == saveSlotId) {
        clearLastSaveSlot();
    }
    return true;
}

QString SaveSlotService::lastSaveSlotId() const {
    const QSettings settings;
    const QString storedSaveSlotId = settings.value(QString::fromLatin1(LastSaveSlotSettingsKey)).toString();
    if (saveSlotExists(storedSaveSlotId)) {
        return storedSaveSlotId;
    }
    const QString defaultSlotId = SaveSlotPaths::defaultSaveSlotId();
    return saveSlotExists(defaultSlotId) ? defaultSlotId : QString();
}

void SaveSlotService::rememberLastSaveSlot(const QString& saveSlotId) const {
    SaveSlotPaths::requireValidSaveSlotId(saveSlotId);
    QSettings settings;
    settings.setValue(QString::fromLatin1(LastSaveSlotSettingsKey), saveSlotId);
}

void SaveSlotService::clearLastSaveSlot() const {
    QSettings settings;
    settings.remove(QString::fromLatin1(LastSaveSlotSettingsKey));
}
