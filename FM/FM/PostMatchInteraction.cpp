#include "PostMatchInteraction.h"

PostMatchInteraction::PostMatchInteraction(MatchId matchId,
    LeagueId leagueId,
    const Date& date,
    TeamId homeId,
    TeamId awayId,
    int homeGoals,
    int awayGoals,
    int matchweek)
    : GameInteraction(Kind::PostMatch, true),
    matchId(matchId),
    leagueId(leagueId),
    date(date),
    homeId(homeId),
    awayId(awayId),
    homeGoals(homeGoals),
    awayGoals(awayGoals),
    matchweek(matchweek) {
}

MatchId PostMatchInteraction::getMatchId() const {
    return matchId;
}

LeagueId PostMatchInteraction::getLeagueId() const {
    return leagueId;
}

const Date& PostMatchInteraction::getDate() const {
    return date;
}

TeamId PostMatchInteraction::getHomeId() const {
    return homeId;
}

TeamId PostMatchInteraction::getAwayId() const {
    return awayId;
}

int PostMatchInteraction::getHomeGoals() const {
    return homeGoals;
}

int PostMatchInteraction::getAwayGoals() const {
    return awayGoals;
}

int PostMatchInteraction::getMatchweek() const {
    return matchweek;
}