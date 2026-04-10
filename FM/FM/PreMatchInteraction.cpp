#include "PreMatchInteraction.h"

PreMatchInteraction::PreMatchInteraction(LeagueId leagueId,
    const Date& date,
    TeamId homeId,
    TeamId awayId,
    int matchweek)
    : GameInteraction(Kind::PreMatch, true),
    leagueId(leagueId),
    date(date),
    homeId(homeId),
    awayId(awayId),
    matchweek(matchweek) {
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