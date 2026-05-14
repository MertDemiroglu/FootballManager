#include "fm/data/WorldBootstrapService.h"

#include "fm/core/World.h"
#include "fm/data/SqliteBootstrapDatabaseInitializer.h"
#include "fm/data/SqliteBootstrapRepository.h"
#include "fm/data/SqliteLeagueRulesRepository.h"
#include "fm/data/WorldBootstrapLoader.h"

void WorldBootstrapService::loadIntoWorldFromSqlite(World& world, const std::string& dbPath, int initialSeasonYear) {
    SqliteBootstrapRepository repository(dbPath);
    SqliteLeagueRulesRepository rulesRepository(dbPath);
    WorldBootstrapLoader loader(repository, rulesRepository);
    loader.load(world, initialSeasonYear);
}

void WorldBootstrapService::initializeAndLoadIntoWorldFromSqlite(
    World& world,
    const std::string& dbPath,
    const std::string& schemaSqlPath,
    const std::string& seedSqlPath,
    int initialSeasonYear) {
    SqliteBootstrapDatabaseInitializer::initializeWithSeed(dbPath, schemaSqlPath, seedSqlPath);
    loadIntoWorldFromSqlite(world, dbPath, initialSeasonYear);
}

void WorldBootstrapService::resetAndLoadIntoWorldFromSqlite(
    World& world,
    const std::string& dbPath,
    const std::string& schemaSqlPath,
    const std::string& seedSqlPath,
    int initialSeasonYear) {
    SqliteBootstrapDatabaseInitializer::resetWithSeed(dbPath, schemaSqlPath, seedSqlPath);
    loadIntoWorldFromSqlite(world, dbPath, initialSeasonYear);
}
