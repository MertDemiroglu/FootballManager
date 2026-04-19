#include "fm/competition/LeagueRules.h"
#include "fm/competition/SeasonPlan.h"
#include "fm/core/World.h"
#include "fm/data/SqliteBootstrapDatabaseInitializer.h"
#include "fm/data/WorldBootstrapService.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: fm_sqlite_bootstrap_smoke <sqlite_db_path> [--init <schema.sql> [seed.sql]]\n";
        return EXIT_FAILURE;
    }

    try {
        const std::string dbPath = argv[1];
        const LeagueRules rules = LeagueRules::makeSuperLig();
        const SeasonPlan seasonPlan = SeasonPlan::build(2025, rules);
        World world;

        if (argc >= 4 && std::string(argv[2]) == "--init") {
            const std::string schemaPath = argv[3];
            if (argc >= 5) {
                const std::string seedPath = argv[4];
                WorldBootstrapService::initializeAndLoadIntoWorldFromSqlite(world, dbPath, schemaPath, seedPath, rules, seasonPlan);
            }
            else {
                SqliteBootstrapDatabaseInitializer::initialize(dbPath, schemaPath);
                WorldBootstrapService::loadIntoWorldFromSqlite(world, dbPath, rules, seasonPlan);
            }
        }
        else {
            WorldBootstrapService::loadIntoWorldFromSqlite(world, dbPath, rules, seasonPlan);
        }

        std::size_t leagueCount = 0;
        std::size_t teamCount = 0;
        std::size_t playerCount = 0;

        world.forEachLeagueContext([&](const LeagueContext& context) {
            ++leagueCount;
            for (const auto& [teamId, team] : context.getLeague().getTeams()) {
                (void)teamId;
                ++teamCount;
                playerCount += team->getPlayers().size();
            }
        });

        std::cout << "sqlite bootstrap loaded leagues=" << leagueCount
                  << " teams=" << teamCount
                  << " players=" << playerCount
                  << "\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& ex) {
        std::cerr << "sqlite bootstrap failed: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}
