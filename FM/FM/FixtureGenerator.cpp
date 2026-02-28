#include "FixtureGenerator.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "League.h"

namespace {
    Date addDays(Date date, int days) {
        for (int i = 0; i < days; ++i) {
            date.advanceDay();
        }
        return date;
    }
}

void FixtureGenerator::generateSeasonFixture(League& league, const Date& startDate) const {
    std::vector<std::string> teamNames;
    teamNames.reserve(league.getTeams().size());

    for (const auto& [teamName, team] : league.getTeams()) {
        (void)team;
        teamNames.push_back(teamName);
    }

    std::sort(teamNames.begin(), teamNames.end());

    if (teamNames.size() != 18) {
        throw std::runtime_error("FixtureGenerator requires exactly 18 teams.");
    }

    const std::size_t teamCount = teamNames.size();
    const std::size_t matchesPerMatchday = teamCount / 2;
    const std::size_t roundsPerHalf = teamCount - 1;

    std::vector<std::string> rotatingOrder = teamNames;
    std::vector<std::vector<std::pair<std::string, std::string>>> firstHalf;
    firstHalf.reserve(roundsPerHalf);

    for (std::size_t round = 0; round < roundsPerHalf; ++round) {
        std::vector<std::pair<std::string, std::string>> pairings;
        pairings.reserve(matchesPerMatchday);

        for (std::size_t i = 0; i < matchesPerMatchday; ++i) {
            const std::string& left = rotatingOrder[i];
            const std::string& right = rotatingOrder[teamCount - 1 - i];

            bool swapHomeAway = ((round + i) % 2 == 1);
            if (i == 0) {
                swapHomeAway = (round % 2 == 1);
            }

            if (swapHomeAway) {
                pairings.emplace_back(right, left);
            }
            else {
                pairings.emplace_back(left, right);
            }
        }

        firstHalf.push_back(std::move(pairings));

        std::vector<std::string> nextOrder;
        nextOrder.reserve(teamCount);
        nextOrder.push_back(rotatingOrder[0]);
        nextOrder.push_back(rotatingOrder.back());

        for (std::size_t i = 1; i < teamCount - 1; ++i) {
            nextOrder.push_back(rotatingOrder[i]);
        }

        rotatingOrder = std::move(nextOrder);
    }

    league.clearFixture();

    Date matchdayDate = startDate;

    for (const auto& roundPairings : firstHalf) {
        for (const auto& [home, away] : roundPairings) {
            league.addFixtureMatch(matchdayDate, home, away);
        }
        matchdayDate = addDays(matchdayDate, 7);
    }

    for (const auto& roundPairings : firstHalf) {
        for (const auto& [home, away] : roundPairings) {
            league.addFixtureMatch(matchdayDate, away, home);
        }
        matchdayDate = addDays(matchdayDate, 7);
    }
}