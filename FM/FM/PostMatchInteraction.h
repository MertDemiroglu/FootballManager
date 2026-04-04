#pragma once

#include "Date.h"
#include "GameInteraction.h"
#include "Types.h"

class PostMatchInteraction : public GameInteraction {
public:
    PostMatchInteraction(LeagueId leagueId, const Date& date, TeamId homeId, TeamId awayId, int homeGoals, int awayGoals, int matchweek);

    LeagueId getLeagueId() const;
    const Date& getDate() const;
    TeamId getHomeId() const;
    TeamId getAwayId() const;
    int getHomeGoals() const;
    int getAwayGoals() const;
    int getMatchweek() const;

private:
    LeagueId leagueId;
    Date date;
    TeamId homeId;
    TeamId awayId;
    int homeGoals;
    int awayGoals;
    int matchweek;
};