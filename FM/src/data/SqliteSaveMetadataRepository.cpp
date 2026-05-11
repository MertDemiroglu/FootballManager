#include "fm/data/SqliteSaveMetadataRepository.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr int MetadataRowId = 1;

    std::string currentUtcTimestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm utcTime{};
#if defined(_WIN32)
        gmtime_s(&utcTime, &nowTime);
#else
        gmtime_r(&nowTime, &utcTime);
#endif

        std::ostringstream output;
        output << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
        return output.str();
    }

    SaveMetadata normalizedForWrite(SaveMetadata metadata) {
        const std::string now = currentUtcTimestamp();
        if (metadata.saveSlotId.empty()) {
            metadata.saveSlotId = "default";
        }
        if (metadata.saveName.empty()) {
            metadata.saveName = "Default Save";
        }
        if (metadata.managerName.empty()) {
            metadata.managerName = "Manager";
        }
        if (metadata.currentDate.empty()) {
            metadata.currentDate = "2025-07-01";
        }
        if (metadata.createdAtUtc.empty()) {
            metadata.createdAtUtc = now;
        }
        if (metadata.updatedAtUtc.empty()) {
            metadata.updatedAtUtc = metadata.createdAtUtc;
        }
        return metadata;
    }

    void bindMetadata(SqliteStatement& statement, const SaveMetadata& metadata) {
        statement.bindInt(1, MetadataRowId);
        statement.bindText(2, metadata.saveSlotId);
        statement.bindText(3, metadata.saveName);
        statement.bindText(4, metadata.managerName);
        statement.bindInt(5, static_cast<int>(metadata.managedLeagueId));
        statement.bindInt(6, static_cast<int>(metadata.managedTeamId));
        statement.bindText(7, metadata.currentDate);
        statement.bindText(8, metadata.createdAtUtc);
        statement.bindText(9, metadata.updatedAtUtc);
        statement.bindInt(10, metadata.schemaVersion);
        statement.bindInt(11, metadata.worldVersion);
    }

    SaveMetadata readMetadata(SqliteStatement& statement) {
        SaveMetadata metadata;
        metadata.saveSlotId = statement.columnText(0);
        metadata.saveName = statement.columnText(1);
        metadata.managerName = statement.columnText(2);
        metadata.managedLeagueId = static_cast<LeagueId>(statement.columnInt(3));
        metadata.managedTeamId = static_cast<TeamId>(statement.columnInt(4));
        metadata.currentDate = statement.columnText(5);
        metadata.createdAtUtc = statement.columnText(6);
        metadata.updatedAtUtc = statement.columnText(7);
        metadata.schemaVersion = statement.columnInt(8);
        metadata.worldVersion = statement.columnInt(9);
        return metadata;
    }
}

SqliteSaveMetadataRepository::SqliteSaveMetadataRepository(const std::string& databasePath)
    : database(databasePath) {
    database.execute(
        "CREATE TABLE IF NOT EXISTS save_metadata ("
        "id INTEGER PRIMARY KEY CHECK (id = 1),"
        "save_slot_id TEXT NOT NULL,"
        "save_name TEXT NOT NULL,"
        "manager_name TEXT NOT NULL,"
        "managed_league_id INTEGER NOT NULL DEFAULT 0,"
        "managed_team_id INTEGER NOT NULL DEFAULT 0,"
        "current_date TEXT NOT NULL,"
        "created_at_utc TEXT NOT NULL,"
        "updated_at_utc TEXT NOT NULL,"
        "schema_version INTEGER NOT NULL DEFAULT 1,"
        "world_version INTEGER NOT NULL DEFAULT 1"
        ");");
    database.execute("PRAGMA user_version = 1;");
}

bool SqliteSaveMetadataRepository::exists() const {
    SqliteStatement statement = database.prepare("SELECT COUNT(*) FROM save_metadata WHERE id = ?");
    statement.bindInt(1, MetadataRowId);
    if (!statement.stepRow()) {
        return false;
    }
    return statement.columnInt(0) > 0;
}

SaveMetadata SqliteSaveMetadataRepository::load() const {
    SqliteStatement statement = database.prepare(
        "SELECT save_slot_id, save_name, manager_name, managed_league_id, managed_team_id, "
        "current_date, created_at_utc, updated_at_utc, schema_version, world_version "
        "FROM save_metadata WHERE id = ?");
    statement.bindInt(1, MetadataRowId);
    if (!statement.stepRow()) {
        throw std::runtime_error("save metadata row does not exist");
    }
    return readMetadata(statement);
}

void SqliteSaveMetadataRepository::insertDefault(const SaveMetadata& metadata) const {
    const SaveMetadata normalized = normalizedForWrite(metadata);
    SqliteStatement statement = database.prepare(
        "INSERT OR IGNORE INTO save_metadata ("
        "id, save_slot_id, save_name, manager_name, managed_league_id, managed_team_id, "
        "current_date, created_at_utc, updated_at_utc, schema_version, world_version"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    bindMetadata(statement, normalized);
    statement.stepDone();
}

void SqliteSaveMetadataRepository::upsert(const SaveMetadata& metadata) const {
    const SaveMetadata normalized = normalizedForWrite(metadata);
    SqliteStatement statement = database.prepare(
        "INSERT INTO save_metadata ("
        "id, save_slot_id, save_name, manager_name, managed_league_id, managed_team_id, "
        "current_date, created_at_utc, updated_at_utc, schema_version, world_version"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "save_slot_id = excluded.save_slot_id, "
        "save_name = excluded.save_name, "
        "manager_name = excluded.manager_name, "
        "managed_league_id = excluded.managed_league_id, "
        "managed_team_id = excluded.managed_team_id, "
        "current_date = excluded.current_date, "
        "created_at_utc = excluded.created_at_utc, "
        "updated_at_utc = excluded.updated_at_utc, "
        "schema_version = excluded.schema_version, "
        "world_version = excluded.world_version");
    bindMetadata(statement, normalized);
    statement.stepDone();
}

void SqliteSaveMetadataRepository::updateCurrentDate(const std::string& currentDate) const {
    SqliteStatement statement = database.prepare("UPDATE save_metadata SET current_date = ? WHERE id = ?");
    statement.bindText(1, currentDate);
    statement.bindInt(2, MetadataRowId);
    statement.stepDone();
}

void SqliteSaveMetadataRepository::updateManagerName(const std::string& managerName) const {
    SqliteStatement statement = database.prepare("UPDATE save_metadata SET manager_name = ? WHERE id = ?");
    statement.bindText(1, managerName.empty() ? "Manager" : managerName);
    statement.bindInt(2, MetadataRowId);
    statement.stepDone();
}

void SqliteSaveMetadataRepository::updateManagedClub(LeagueId leagueId, TeamId teamId) const {
    SqliteStatement statement = database.prepare(
        "UPDATE save_metadata SET managed_league_id = ?, managed_team_id = ? WHERE id = ?");
    statement.bindInt(1, static_cast<int>(leagueId));
    statement.bindInt(2, static_cast<int>(teamId));
    statement.bindInt(3, MetadataRowId);
    statement.stepDone();
}

void SqliteSaveMetadataRepository::touchUpdatedAt() const {
    SqliteStatement statement = database.prepare("UPDATE save_metadata SET updated_at_utc = ? WHERE id = ?");
    statement.bindText(1, currentUtcTimestamp());
    statement.bindInt(2, MetadataRowId);
    statement.stepDone();
}
