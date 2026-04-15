#include "PreMatchInteraction.h"

#include <utility>

PreMatchInteraction::PreMatchInteraction(MatchId matchId,
    LeagueId leagueId,
    const Date& date,
    TeamId homeId,
    TeamId awayId,
    int matchweek,
    TeamSheet homeSheet,
    TeamSheet awaySheet)
    : GameInteraction(Kind::PreMatch, true),
    matchId(matchId),
    leagueId(leagueId),
    date(date),
    homeId(homeId),
    awayId(awayId),
    matchweek(matchweek),
    homeSheet(std::move(homeSheet)),
    awaySheet(std::move(awaySheet)) {
}

MatchId PreMatchInteraction::getMatchId() const {
    return matchId;
}

LeagueId PreMatchInteraction::getLeagueId() const {
    return leagueId;
}

const Date& PreMatchInteraction::getDate() const {
    return date;
}

TeamId PreMatchInteraction::getHomeId() const {
    return homeId;
}

TeamId PreMatchInteraction::getAwayId() const {
    return awayId;
}

int PreMatchInteraction::getMatchweek() const {
    return matchweek;
}

const TeamSheet& PreMatchInteraction::getHomeSheet() const {
    return homeSheet;
}

const TeamSheet& PreMatchInteraction::getAwaySheet() const {
    return awaySheet;
}
