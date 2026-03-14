#include <iostream>
#include <stdexcept>

#include "Game.h"
#include "League.h"
#include "Date.h"
#include "MatchScheduler.h"

static void assertOrThrow(bool cond, const char* msg) {
    if (!cond) throw std::runtime_error(msg);
}

int main() {
    try {
        Game game;

        const int simulationDays = 365 * 4;

        int seasonStarts = 0;
        int lastSeasonStartYear = game.getDate().getYear() - 1;

        std::cout << "[Boot] Year=" << game.getDate().getYear()
            << " teams=" << game.getLeague().getTeams().size()
            << " startDate="
            << static_cast<int>(game.getDate().getMonth()) << "/" << game.getDate().getDay()
            << "\n";

        for (int day = 0; day < simulationDays; ++day) {
            game.updateDaily();

            const Date& d = game.getDate();

            if (d.getMonth() == Month::July && d.getDay() == 1 && d.getYear() != lastSeasonStartYear) {
                lastSeasonStartYear = d.getYear();
                seasonStarts++;

                const League& league = game.getLeague();

                const int teams = static_cast<int>(league.getTeams().size());
                const bool fixtureGenerated = league.isSeasonFixtureGenerated();
                const int totalMatches = league.debugTotalFixtureMatches();
                const int matchdayCount = league.debugMatchdayCount();

                std::cout << "\n[SeasonStart] Year=" << d.getYear()
                    << " teams=" << teams
                    << " fixtureGenerated=" << (fixtureGenerated ? "true" : "false")
                    << " matchdays=" << matchdayCount
                    << " totalMatches=" << totalMatches
                    << "\n";

                assertOrThrow(teams == 18, "Team count must be 18 at season start.");
                assertOrThrow(fixtureGenerated, "Fixture should be marked generated at season start.");
                assertOrThrow(matchdayCount == 34, "Matchday tracking count must be 34.");
                assertOrThrow(totalMatches == 306, "Total fixture matches must be 306.");

                for (int md = 1; md <= 34; ++md) {
                    assertOrThrow(league.tryGetMatchdayEndDate(md).has_value(), "Each matchday should have an end date.");
                }
            }
        }

        std::cout << "\nSimulation completed successfully.\n";
        std::cout << "Season starts observed: " << seasonStarts << "\n";
        std::cout << "Total generated match events: "
            << game.getMatchScheduler().debugGeneratedMatchEvents() << "\n";

        assertOrThrow(game.getState() == GameState::PostSeason || game.getState() == GameState::PreSeason,
            "Expected PostSeason or next PreSeason by simulation end.");

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "\nTEST FAILED: " << ex.what() << "\n";
        return 1;
    }
}
