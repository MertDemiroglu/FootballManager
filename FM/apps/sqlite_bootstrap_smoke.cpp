#include "fm/core/Game.h"
#include "fm/core/LeagueContext.h"
#include "fm/competition/League.h"
#include "fm/roster/Team.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
    const char* usage() {
        return "usage: fm_sqlite_bootstrap_smoke <sqlite_db_path> "
               "[--open-existing | --create-if-missing <schema.sql> <seed.sql> | --reset <schema.sql> <seed.sql>]";
    }

    GameBootstrapOptions buildBootstrapOptions(int argc, char** argv) {
        if (argc < 2) {
            throw std::invalid_argument(usage());
        }

        GameBootstrapOptions options;
        options.mode = GameBootstrapMode::Sqlite;
        options.sqliteDbPath = argv[1];

        if (argc == 2) {
            options.databaseOpenMode = DatabaseOpenMode::OpenExisting;
            return options;
        }

        const std::string modeArg = argv[2];
        if (modeArg == "--open-existing" && argc == 3) {
            options.databaseOpenMode = DatabaseOpenMode::OpenExisting;
            return options;
        }
        if (modeArg == "--create-if-missing" && argc == 5) {
            options.databaseOpenMode = DatabaseOpenMode::CreateFromSeedIfMissing;
            options.sqliteSchemaPath = argv[3];
            options.sqliteSeedPath = argv[4];
            return options;
        }
        if (modeArg == "--reset" && argc == 5) {
            options.databaseOpenMode = DatabaseOpenMode::ResetFromSeed;
            options.sqliteSchemaPath = argv[3];
            options.sqliteSeedPath = argv[4];
            return options;
        }

        throw std::invalid_argument(usage());
    }
}

int main(int argc, char** argv) {
    try {
        Game game(buildBootstrapOptions(argc, argv));

        std::size_t leagueCount = 0;
        std::size_t teamCount = 0;
        std::size_t playerCount = 0;

        game.forEachLeagueContext([&](const LeagueContext& context) {
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
