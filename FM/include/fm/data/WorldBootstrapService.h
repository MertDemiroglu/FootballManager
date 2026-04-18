#pragma once

#include "fm/competition/LeagueRules.h"
#include "fm/competition/SeasonPlan.h"

#include <string>

class World;

class WorldBootstrapService {
public:
    static void loadIntoWorldFromSqlite(World& world, const std::string& dbPath, const LeagueRules& rules, const SeasonPlan& seasonPlan);

    static void initializeAndLoadIntoWorldFromSqlite(
        World& world,
        const std::string& dbPath,
        const std::string& schemaSqlPath,
        const std::string& seedSqlPath,
        const LeagueRules& rules,
        const SeasonPlan& seasonPlan);
};
