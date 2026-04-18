#pragma once

#include "fm/competition/LeagueRules.h"
#include "fm/competition/SeasonPlan.h"

#include <string>

class World;

class WorldBootstrapService {
public:
    static World createWorldFromSqlite(const std::string& dbPath, const LeagueRules& rules, const SeasonPlan& seasonPlan);

    static World createWorldFromSqliteWithInitialization(
        const std::string& dbPath,
        const std::string& schemaSqlPath,
        const std::string& seedSqlPath,
        const LeagueRules& rules,
        const SeasonPlan& seasonPlan);
};
