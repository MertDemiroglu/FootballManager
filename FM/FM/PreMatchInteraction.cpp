#include "PreMatchInteraction.h"

PreMatchInteraction::PreMatchInteraction(MatchId matchId,
    LeagueId leagueId,
    const Date& date,
    TeamId homeId,
    TeamId awayId,
    int matchweek)
    : GameInteraction(Kind::PreMatch, true),
    matchId(matchId),
    leagueId(leagueId),
    date(date),
    homeId(homeId),
    awayId(awayId),
    matchweek(matchweek) {
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