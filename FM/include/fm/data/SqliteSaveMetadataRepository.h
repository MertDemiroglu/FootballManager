#pragma once

#include "fm/common/Types.h"
#include "fm/data/SaveMetadata.h"
#include "fm/data/SqliteDatabase.h"

#include <string>

class SqliteSaveMetadataRepository {
private:
    SqliteDatabase database;

public:
    explicit SqliteSaveMetadataRepository(const std::string& databasePath);

    bool exists() const;
    SaveMetadata load() const;
    void insertDefault(const SaveMetadata& metadata) const;
    void upsert(const SaveMetadata& metadata) const;
    void updateCurrentDate(const std::string& currentDate) const;
    void updateManagerName(const std::string& managerName) const;
    void updateManagedClub(LeagueId leagueId, TeamId teamId) const;
    void touchUpdatedAt() const;
};
