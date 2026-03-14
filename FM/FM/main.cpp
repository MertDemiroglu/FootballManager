#include <iostream>
#include <stdexcept>

#include "Game.h"
#include "League.h"
#include "Date.h"
#include "MatchScheduler.h"

static void assertOrThrow(bool cond, const char* msg) {
    if (!cond) throw std::runtime_error(msg);
}

static const char* stateToStr(int s) {
    switch (s) {
    case 0: return "PreSeason";
    case 1: return "InSeason";
    case 2: return "PostSeason";
    default: return "Unknown";
    }
}

int main() {
    try {
        Game game;

        const int simulationDays = 1461;

        int seasonStarts = 0;

        int lastSeasonStartYear = game.getDate().getYear() - 1;

        int lastSeasonMatchEvents = 0;

        std::cout << "[Boot] Year=" << game.getDate().getYear()
            << " teams=" << game.getLeague().getTeams().size()
            << " startDate="
            << static_cast<int>(game.getDate().getMonth()) << "/" << game.getDate().getDay()
            << "\n";

        for (int day = 0; day < simulationDays; ++day) {
            game.updateDaily();

            const Date& d = game.getDate();

        
            if (day % 50 == 0) {
                std::cout << "[Tick] simDay=" << day
                    << " dateYear=" << d.getYear()
                    << " month=" << static_cast<int>(d.getMonth())
                    << " day=" << d.getDay()
                    << "\n";
            }

    
            if (d.getMonth() == Month::July && d.getDay() == 1 && d.getYear() != lastSeasonStartYear) {
                lastSeasonStartYear = d.getYear();
                seasonStarts++;

                const League& league = game.getLeague();

                const int teams = static_cast<int>(league.getTeams().size());
                const bool fixtureGenerated = league.isSeasonFixtureGenerated();
                const int fixtureDays = league.debugFixtureDayCount();
                const int totalMatches = league.debugTotalFixtureMatches();

           
                const int stateInt = -1;

                const int totalGeneratedMatchEvents = game.getMatchScheduler().debugGeneratedMatchEvents();
                const int generatedThisSeason = totalGeneratedMatchEvents - lastSeasonMatchEvents;
                lastSeasonMatchEvents = totalGeneratedMatchEvents;

                std::cout << "\n[SeasonStart] Year=" << d.getYear()
                    << " State=" << stateToStr(stateInt)
                    << " teams=" << teams
                    << " fixtureGenerated=" << (fixtureGenerated ? "true" : "false")
                    << " fixtureDays=" << fixtureDays
                    << " totalMatches=" << totalMatches
                    << " matchEventsSinceLastSeason=" << generatedThisSeason
                    << "\n";

                assertOrThrow(teams == 18, "Team count must be 18 at season start.");
                assertOrThrow(fixtureGenerated, "Fixture should be marked generated at season start.");
                assertOrThrow(fixtureDays == 34, "Fixture day count must be 34.");
                assertOrThrow(totalMatches == 306, "Total fixture matches must be 306.");
            }
        }

        std::cout << "\nSimulation completed successfully.\n";
        std::cout << "Season starts observed: " << seasonStarts << "\n";
        std::cout << "Total generated match events: "
            << game.getMatchScheduler().debugGeneratedMatchEvents() << "\n";

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "\nTEST FAILED: " << ex.what() << "\n";
        return 1;
    }
}