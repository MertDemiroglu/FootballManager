    #include"FixtureGenerator.h"

#include<algorithm>
#include<stdexcept>
#include<string>
#include<utility>
#include<vector>

#include"fm/common/DateUtils.h"
#include"League.h"

void FixtureGenerator::generateSeasonFixture(League& league, const SeasonPlan& plan, const LeagueRules& rules, const std::function<MatchId()>& matchIdAllocator) const {
    if (!matchIdAllocator) {
        throw std::invalid_argument("matchIdAllocator cannot be empty");
    }
    std::vector<std::pair<std::string, TeamId>> teamsByName;
    teamsByName.reserve(league.getTeams().size());


    for (const auto& [teamId, team] : league.getTeams()) {
        (void)teamId;
        teamsByName.emplace_back(team->getName(), team->getId());
    }

    std::sort(teamsByName.begin(), teamsByName.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
        });

    if (static_cast<int>(teamsByName.size()) != rules.teamCount) {
        throw std::runtime_error("FixtureGenerator team count does not match LeagueRules.");
    }

    const int matchesPerMatchday = rules.teamCount / 2;
    if (static_cast<int>(rules.matchdayDistributionOffsets.size()) != matchesPerMatchday) {
        throw std::runtime_error("matchdayDistributionOffsets size must equal matchesPerMatchday.");
    }

    const std::size_t teamCount = teamsByName.size();
    const std::size_t roundsPerHalf = static_cast<std::size_t>(rules.firstHalfRounds);

    std::vector<TeamId> rotatingOrder;
    rotatingOrder.reserve(teamCount);
    for (const auto& [teamName, teamId] : teamsByName) {
        (void)teamName;
        rotatingOrder.push_back(teamId);
    }

    std::vector<std::vector<std::pair<TeamId, TeamId>>> firstHalf;
    firstHalf.reserve(roundsPerHalf);

    for (std::size_t round = 0; round < roundsPerHalf; ++round) {
        std::vector<std::pair<TeamId, TeamId>> pairings;
        pairings.reserve(static_cast<std::size_t>(matchesPerMatchday));

        for (int i = 0; i < matchesPerMatchday; ++i) {
            const TeamId left = rotatingOrder[static_cast<std::size_t>(i)];
            const TeamId right = rotatingOrder[teamCount - 1 - static_cast<std::size_t>(i)];
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

        std::vector<TeamId> nextOrder;
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
            const auto& [homeId, awayId] = roundPairings[static_cast<std::size_t>(i)];
            const Date matchDate = DateUtils::addDays(matchdayDate, rules.matchdayDistributionOffsets[static_cast<std::size_t>(i)]);
            league.addFixtureMatch(matchIdAllocator(), matchdayIndex, matchDate, homeId, awayId);
        }
        ++matchdayIndex;
        matchdayDate = DateUtils::addDays(matchdayDate, rules.matchSpacingDays);
    }

    if (rules.winterBreakEnabled) {
        matchdayDate = DateUtils::addDays(matchdayDate, rules.winterBreakLengthDays);
    }

    for (const auto& roundPairings : firstHalf) {
            for (int i = 0; i < matchesPerMatchday; ++i) {
                const auto& [homeId, awayId] = roundPairings[static_cast<std::size_t>(i)];
                const Date matchDate = DateUtils::addDays(matchdayDate, rules.matchdayDistributionOffsets[static_cast<std::size_t>(i)]);
                league.addFixtureMatch(matchIdAllocator(), matchdayIndex, matchDate, awayId, homeId);
            }
            ++matchdayIndex;
            matchdayDate = DateUtils::addDays(matchdayDate, rules.matchSpacingDays);
        }
    
}