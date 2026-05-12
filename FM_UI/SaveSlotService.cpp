#include"SaveSlotService.h"

#include"SaveSlotPaths.h"

#include"fm/data/SaveMetadata.h"
#include"fm/data/SqliteGameStateRepository.h"
#include"fm/data/SqliteDatabase.h"
#include"fm/data/SqliteSaveMetadataRepository.h"

#include<QDateTime>
#include<QDebug>
#include<QDir>
#include<QFileInfo>
#include<QRegularExpression>
#include<QSettings>

#include<algorithm>
#include<exception>
#include<iomanip>
#include<sstream>
#include<stdexcept>
#include<vector>

namespace {
    constexpr const char* LastSaveSlotSettingsKey = "saveSlots/lastSaveSlotId";

    QString fromStd(const std::string& value) {
        return QString::fromStdString(value);
    }

    QString dateToIsoString(const Date& date) {
        std::ostringstream output;
        output << std::setw(4) << std::setfill('0') << date.getYear() << "-"
            << std::setw(2) << std::setfill('0') << static_cast<int>(date.getMonth()) << "-"
            << std::setw(2) << std::setfill('0') << date.getDay();
        return QString::fromStdString(output.str());
    }

    bool sameDate(const Date& lhs, const Date& rhs) {
        return lhs.getYear() == rhs.getYear()
            && lhs.getMonth() == rhs.getMonth()
            && lhs.getDay() == rhs.getDay();
    }

    bool dateIsBefore(const Date& lhs, const Date& rhs) {
        return lhs < rhs;
    }

    bool runtimeStateLooksConsistent(
        const Date& currentDate,
        const std::vector<PersistedLeagueRuntimeState>& leagueStates,
        const std::vector<PersistedFixtureState>& fixtures,
        const std::vector<PersistedPlayerRuntimeState>& playerStates) {
        if (leagueStates.empty() || fixtures.empty() || playerStates.empty()) {
            return false;
        }

        for (const PersistedLeagueRuntimeState& state : leagueStates) {
            if (state.leagueId == 0 || state.seasonYear < 0) {
                return false;
            }

            const Date preseasonStart(state.seasonYear, Month::July, 1);
            const Date nextPreseasonStart(state.seasonYear + 1, Month::July, 1);
            if (dateIsBefore(currentDate, preseasonStart)) {
                return false;
            }
            if (!dateIsBefore(currentDate, nextPreseasonStart) && !sameDate(currentDate, nextPreseasonStart)) {
                return false;
            }

            bool sawFixtureForLeague = false;
            for (const PersistedFixtureState& fixture : fixtures) {
                if (fixture.leagueId != state.leagueId) {
                    continue;
                }
                sawFixtureForLeague = true;
                if (fixture.seasonYear != state.seasonYear
                    || fixture.matchId == 0
                    || fixture.matchweek <= 0
                    || fixture.homeTeamId == 0
                    || fixture.awayTeamId == 0
                    || fixture.date.getYear() < state.seasonYear
                    || fixture.date.getYear() > state.seasonYear + 1) {
                    return false;
                }
            }

            if (state.fixtureGenerated && !sawFixtureForLeague) {
                return false;
            }
        }

        return true;
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

    bool isUntouchedDefaultSave(const QString& folderSaveSlotId, const SaveSlotInfo& info) {
        return folderSaveSlotId == SaveSlotPaths::defaultSaveSlotId()
            && info.saveSlotId == SaveSlotPaths::defaultSaveSlotId()
            && info.saveName == QStringLiteral("Default Save")
            && info.managerName == QStringLiteral("Manager")
            && info.managedLeagueId == 0
            && info.managedTeamId == 0;
    }

    void resolveManagedClubNames(const QString& dbPath, SaveSlotInfo& info) {
        if (info.managedLeagueId <= 0 || info.managedTeamId <= 0) {
            return;
        }

        try {
            SqliteDatabase database(dbPath.toStdString());
            SqliteStatement statement = database.prepare(
                "SELECT t.name, l.name "
                "FROM teams t "
                "JOIN leagues l ON l.id = t.league_id "
                "WHERE t.id = ? AND t.league_id = ?");
            statement.bindInt(1, info.managedTeamId);
            statement.bindInt(2, info.managedLeagueId);
            if (statement.stepRow()) {
                info.managedTeamName = fromStd(statement.columnText(0));
                info.managedLeagueName = fromStd(statement.columnText(1));
            }
        } catch (const std::exception& ex) {
            qWarning() << "[SaveSlotService] Could not resolve managed club names for save"
                       << info.saveSlotId << ":" << ex.what();
        }
    }

    bool hasUserFacingSaveMetadata(const QString& saveSlotId) {
        if (!SaveSlotPaths::isValidSaveSlotId(saveSlotId)) {
            return false;
        }

        const QString dbPath = SaveSlotPaths::saveDatabasePath(saveSlotId);
        if (!QFileInfo::exists(dbPath)) {
            return false;
        }

        try {
            SqliteSaveMetadataRepository repository(
                dbPath.toStdString(),
                SaveMetadataRepositoryMode::ReadExisting);
            if (!repository.exists()) {
                return false;
            }
            SaveSlotInfo info = toSaveSlotInfo(repository.load());
            if (info.saveSlotId.isEmpty()) {
                info.saveSlotId = saveSlotId;
            }

            SqliteGameStateRepository runtimeRepository(
                dbPath.toStdString(),
                GameStateRepositoryMode::ReadExisting);
            if (!runtimeRepository.hasGameState()) {
                return false;
            }
            const Date currentDate = runtimeRepository.loadCurrentDate();
            const std::vector<PersistedLeagueRuntimeState> leagueStates = runtimeRepository.loadLeagueRuntimeStates();
            const std::vector<PersistedFixtureState> fixtures = runtimeRepository.loadFixtures();
            const std::vector<PersistedPlayerRuntimeState> playerStates = runtimeRepository.loadPlayerRuntimeStates();
            if (!runtimeStateLooksConsistent(currentDate, leagueStates, fixtures, playerStates)) {
                return false;
            }

            return !isUntouchedDefaultSave(saveSlotId, info);
        } catch (const std::exception& ex) {
            qWarning() << "[SaveSlotService] Last save slot is not readable" << saveSlotId << ":" << ex.what();
            return false;
        }
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
            if (isUntouchedDefaultSave(saveSlotId, info)) {
                qWarning() << "[SaveSlotService] Skipping untouched default save slot:" << saveSlotId;
                continue;
            }

            SqliteGameStateRepository runtimeRepository(
                dbPath.toStdString(),
                GameStateRepositoryMode::ReadExisting);
            if (!runtimeRepository.hasGameState()) {
                qWarning() << "[SaveSlotService] Skipping save without runtime game_state:" << saveSlotId;
                continue;
            }
            const Date currentDate = runtimeRepository.loadCurrentDate();
            const std::vector<PersistedLeagueRuntimeState> leagueStates = runtimeRepository.loadLeagueRuntimeStates();
            const std::vector<PersistedFixtureState> fixtures = runtimeRepository.loadFixtures();
            const std::vector<PersistedPlayerRuntimeState> playerStates = runtimeRepository.loadPlayerRuntimeStates();
            if (!runtimeStateLooksConsistent(currentDate, leagueStates, fixtures, playerStates)) {
                qWarning() << "[SaveSlotService] Skipping inconsistent runtime save slot:" << saveSlotId;
                continue;
            }
            info.currentDate = dateToIsoString(currentDate);
            resolveManagedClubNames(dbPath, info);
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
    if (!hasUserFacingSaveMetadata(saveSlotId)) {
        throw std::invalid_argument("save slot runtime state is missing or unreadable: " + saveSlotId.toStdString());
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
    if (hasUserFacingSaveMetadata(storedSaveSlotId)) {
        return storedSaveSlotId;
    }
    return QString();
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
