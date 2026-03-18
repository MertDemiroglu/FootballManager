#include"MatchEvent.h"
#include"Team.h"
#include"Game.h"

#include<algorithm>
#include <cstdint>
#include<utility>
#include<functional>
#include<random>

namespace {
    std::uint32_t makeMatchSeed(TeamId homeId, TeamId awayId, const Date& date) {
        std::uint32_t seed = 2166136261u;

        auto mix = [&seed](std::uint32_t value) {
            seed ^= value;
            seed *= 16777619u;
            };

        mix(static_cast<std::uint32_t>(homeId));
        mix(static_cast<std::uint32_t>(awayId));
        mix(static_cast<std::uint32_t>(date.getYear()));
        mix(static_cast<std::uint32_t>(date.getMonth()));
        mix(static_cast<std::uint32_t>(date.getDay()));
        mix(static_cast<std::uint32_t>(date.getDayOfWeek()));

        return seed;
    }

    MatchResult buildStrengthBasedResult(const Team& homeTeam, const Team& awayTeam, const Date& date) {
        const int homeRating = homeTeam.calculateTeamRating();
        const int awayRating = awayTeam.calculateTeamRating();
        const int ratingDiff = homeRating - awayRating;

        const double homeAdvantage = 0.25;

        double homeExpectedGoals = 1.20 + homeAdvantage + (ratingDiff / 40.0);
        double awayExpectedGoals = 1.00 - (ratingDiff / 45.0);

        homeExpectedGoals = std::clamp(homeExpectedGoals, 0.2, 3.5);
        awayExpectedGoals = std::clamp(awayExpectedGoals, 0.2, 3.0);

        std::mt19937 rng(makeMatchSeed(homeTeam.getId(), awayTeam.getId(), date));

        std::poisson_distribution<int> homeDist(homeExpectedGoals);
        std::poisson_distribution<int> awayDist(awayExpectedGoals);

        MatchResult result;
        result.homeGoals = std::clamp(homeDist(rng), 0, 6);
        result.awayGoals = std::clamp(awayDist(rng), 0, 6);
        return result;
    }
}

MatchEvent::MatchEvent(TeamId homeId, TeamId awayId, const Date& matchDate, std::string homeName, std::string awayName)
    : homeId(homeId), awayId(awayId), matchDate(matchDate), homeName(std::move(homeName)), awayName(std::move(awayName)) {
}

bool MatchEvent::isBlocking() const { 
    return block; 
}

EventPriority MatchEvent::getPriority() const {
    return priority;
}
void MatchEvent::resolve(Game& game) {
    League& league = game.getLeague();
    const Team* homeTeam = league.findTeamById(homeId);
    const Team* awayTeam = league.findTeamById(awayId);
    if (!homeTeam || !awayTeam) {
        return;
    }
    
    const MatchResult result = buildStrengthBasedResult(*homeTeam, *awayTeam, matchDate);
    league.applyMatchResult(matchDate, homeId, awayId, result);
}
const std::string& MatchEvent::getSendingTeam() const {
    return homeName;
}
const std::string& MatchEvent::getReceivingTeam() const {
    return awayName;
}
EventTargetType MatchEvent::getTargetType() const {
    return type;
}
bool MatchEvent::affectsTeam(const Team* team) const {
    if (!team) {
        return false;
    }
    return team->getId() == homeId || team->getId() == awayId;
}

static const std::string kEmpty{};
const std::string& MatchEvent::getPlayer() const {
    return kEmpty;
}