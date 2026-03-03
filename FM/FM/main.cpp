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

        // 1 sezon görmek istiyorsan 365 yeter ama 2026-07-01'e gelmez.
        // Hem 2025-07-01'i saymak hem de "bir sonraki sezon başlangıcına" ulaşmak için 400 gün mantıklı.
        const int simulationDays = 400;

        int seasonStarts = 0;

        // 🔥 Kritik fix:
        // Başlangıç yılı 2025 ise, 2025-07-01'i de sayabilmek için başlangıç değerini 2024 yap.
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

            // Her 50 günde bir tarih bas (Date sistemini hızlı doğrulamak için)
            if (day % 50 == 0) {
                std::cout << "[Tick] simDay=" << day
                    << " dateYear=" << d.getYear()
                    << " month=" << static_cast<int>(d.getMonth())
                    << " day=" << d.getDay()
                    << "\n";
            }

            // Sezon başlangıcı: July 1 (yıl bazlı)
            if (d.getMonth() == Month::July && d.getDay() == 1 && d.getYear() != lastSeasonStartYear) {
                lastSeasonStartYear = d.getYear();
                seasonStarts++;

                const League& league = game.getLeague();

                const int teams = static_cast<int>(league.getTeams().size());
                const bool fixtureGenerated = league.isSeasonFixtureGenerated();
                const int fixtureDays = league.debugFixtureDayCount();
                const int totalMatches = league.debugTotalFixtureMatches();

                // GameState'i burada okuyamıyoruz çünkü getter yok; şimdilik Unknown basıyoruz.
                // İstersen Game'e: GameState getState() const; ekleyip burada kullanırız.
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

                // Temel doğrulamalar (sadece fixture gerçekten o an üretiliyorsa anlamlı)
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