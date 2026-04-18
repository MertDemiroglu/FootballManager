#include "fm/competition/LeagueRules.h"
#include "fm/competition/SeasonPlan.h"
#include "fm/core/World.h"
#include "fm/data/SqliteBootstrapRepository.h"
#include "fm/data/WorldBootstrapLoader.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: fm_sqlite_bootstrap_smoke <sqlite_db_path>\n";
        return EXIT_FAILURE;
    }

    try {
        const std::string dbPath = argv[1];
        SqliteBootstrapRepository repository(dbPath);
        WorldBootstrapLoader loader(repository);

        World world;
        const LeagueRules rules = LeagueRules::makeSuperLig();
        const SeasonPlan seasonPlan = SeasonPlan::build(2025, rules);

        loader.load(world, rules, seasonPlan);

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
