#include "fm/data/WorldBootstrapService.h"

#include "fm/core/World.h"
#include "fm/data/SqliteBootstrapDatabaseInitializer.h"
#include "fm/data/SqliteBootstrapRepository.h"
#include "fm/data/WorldBootstrapLoader.h"

World WorldBootstrapService::createWorldFromSqlite(const std::string& dbPath, const LeagueRules& rules, const SeasonPlan& seasonPlan) {
    SqliteBootstrapRepository repository(dbPath);
    WorldBootstrapLoader loader(repository);

    World world;
    loader.load(world, rules, seasonPlan);
    return world;
}

World WorldBootstrapService::createWorldFromSqliteWithInitialization(
    const std::string& dbPath,
    const std::string& schemaSqlPath,
    const std::string& seedSqlPath,
    const LeagueRules& rules,
    const SeasonPlan& seasonPlan) {
    SqliteBootstrapDatabaseInitializer::initializeWithSeed(dbPath, schemaSqlPath, seedSqlPath);
    return createWorldFromSqlite(dbPath, rules, seasonPlan);
}
