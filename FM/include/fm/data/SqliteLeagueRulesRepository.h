#pragma once

#include "fm/common/Types.h"
#include "fm/competition/LeagueRules.h"
#include "fm/data/SqliteDatabase.h"

#include <string>
#include <unordered_map>

class SqliteLeagueRulesRepository {
private:
    mutable SqliteDatabase database;

    void ensureSchema() const;

public:
    explicit SqliteLeagueRulesRepository(const std::string& dbPath);

    LeagueRules loadLeagueRules(LeagueId leagueId) const;
    std::unordered_map<LeagueId, LeagueRules> loadAllLeagueRules() const;
};
