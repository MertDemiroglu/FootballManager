#include"FixtureGenerator.h"

#include<algorithm>
#include<stdexcept>
#include<string>
#include<utility>
#include<vector>

#include"DateUtils.h"
#include"League.h"

void FixtureGenerator::generateSeasonFixture(League& league, const SeasonPlan& plan, const LeagueRules& rules) const {
    std::vector<std::string> teamNames;
    teamNames.reserve(league.getTeams().size());

    for (const auto& [teamName, team] : league.getTeams()) {
        (void)team;
        teamNames.push_back(teamName);
    }

    std::sort(teamNames.begin(), teamNames.end());

    if (static_cast<int>(teamNames.size()) != rules.teamCount) {
        throw std::runtime_error("FixtureGenerator team count does not match LeagueRules.");
    }

    const int matchesPerMatchday = rules.teamCount / 2;
    if (static_cast<int>(rules.matchdayDistributionOffsets.size()) != matchesPerMatchday) {
        throw std::runtime_error("matchdayDistributionOffsets size must equal matchesPerMatchday.");
    }

    const std::size_t teamCount = teamNames.size();
    const std::size_t roundsPerHalf = static_cast<std::size_t>(rules.firstHalfRounds);

    std::vector<std::string> rotatingOrder = teamNames;
    std::vector<std::vector<std::pair<std::string, std::string>>> firstHalf;
    firstHalf.reserve(roundsPerHalf);

    for (std::size_t round = 0; round < roundsPerHalf; ++round) {
        std::vector<std::pair<std::string, std::string>> pairings;
        pairings.reserve(static_cast<std::size_t>(matchesPerMatchday));

        for (int i = 0; i < matchesPerMatchday; ++i) {
            const std::string& left = rotatingOrder[static_cast<std::size_t>(i)];
            const std::string& right = rotatingOrder[teamCount - 1 - static_cast<std::size_t>(i)];

            bool swapHomeAway = ((round + static_cast<std::size_t>(i)) % 2 == 1);
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
    league.initializeMatchdayTracking(rules.matchdaysPerSeason);

    Date matchdayDate = plan.getKickoff();
    int matchdayIndex = 1;

    for (const auto& roundPairings : firstHalf) {
        for (int i = 0; i < matchesPerMatchday; ++i) {
            const auto& [home, away] = roundPairings[static_cast<std::size_t>(i)];
            const Date matchDate = DateUtils::addDays(matchdayDate, rules.matchdayDistributionOffsets[static_cast<std::size_t>(i)]);
            league.addFixtureMatch(matchdayIndex, matchDate, home, away);
        }
        ++matchdayIndex;
        matchdayDate = DateUtils::addDays(matchdayDate, rules.matchSpacingDays);
    }

    if (rules.winterBreakEnabled) {
        matchdayDate = DateUtils::addDays(matchdayDate, rules.winterBreakLengthDays);
    }

    for (const auto& roundPairings : firstHalf) {
            for (int i = 0; i < matchesPerMatchday; ++i) {
                const auto& [home, away] = roundPairings[static_cast<std::size_t>(i)];
                const Date matchDate = DateUtils::addDays(matchdayDate, rules.matchdayDistributionOffsets[static_cast<std::size_t>(i)]);
                league.addFixtureMatch(matchdayIndex, matchDate, away, home);
            }
            ++matchdayIndex;
            matchdayDate = DateUtils::addDays(matchdayDate, rules.matchSpacingDays);
        }
    
}