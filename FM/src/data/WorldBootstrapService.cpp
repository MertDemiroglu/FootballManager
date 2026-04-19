#include "fm/data/WorldBootstrapService.h"

#include "fm/core/World.h"
#include "fm/data/SqliteBootstrapDatabaseInitializer.h"
#include "fm/data/SqliteBootstrapRepository.h"
#include "fm/data/WorldBootstrapLoader.h"

void WorldBootstrapService::loadIntoWorldFromSqlite(World& world, const std::string& dbPath, const LeagueRules& rules, const SeasonPlan& seasonPlan) {
    SqliteBootstrapRepository repository(dbPath);
    WorldBootstrapLoader loader(repository);
    loader.load(world, rules, seasonPlan);
}

void WorldBootstrapService::initializeAndLoadIntoWorldFromSqlite(
    World& world,
    const std::string& dbPath,
    const std::string& schemaSqlPath,
    const std::string& seedSqlPath,
    const LeagueRules& rules,
    const SeasonPlan& seasonPlan) {
    SqliteBootstrapDatabaseInitializer::initializeWithSeed(dbPath, schemaSqlPath, seedSqlPath);
    loadIntoWorldFromSqlite(world, dbPath, rules, seasonPlan);
}
