#pragma once

#include "fm/competition/LeagueRules.h"
#include "fm/competition/SeasonPlan.h"

class World;
class SqliteBootstrapRepository;

class WorldBootstrapLoader {
private:
    SqliteBootstrapRepository& repository;

public:
    explicit WorldBootstrapLoader(SqliteBootstrapRepository& repository);

    void load(World& world, const LeagueRules& rules, const SeasonPlan& seasonPlan) const;
};
