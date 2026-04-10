#pragma once

#include"Date.h"
#include"GameInteraction.h"
#include"Types.h"

class PreMatchInteraction : public GameInteraction {
private:
    LeagueId leagueId;
    Date date;
    TeamId homeId;
    TeamId awayId;
    int matchweek;

public:
	PreMatchInteraction(LeagueId leagueId, const Date& date, TeamId homeId, TeamId awayId, int matchweek);

    LeagueId getLeagueId() const;
    const Date& getDate() const;
    TeamId getHomeId() const;
    TeamId getAwayId() const;
    int getMatchweek() const;
};