#pragma once

#include"fm/common/Date.h"
#include"fm/interaction/GameInteraction.h"
#include"fm/common/Types.h"

class PostMatchInteraction : public GameInteraction {
public:
    PostMatchInteraction(MatchId matchId, LeagueId leagueId, const Date& date, TeamId homeId, TeamId awayId, int homeGoals, int awayGoals, int matchweek);

    MatchId getMatchId() const;
    LeagueId getLeagueId() const;
    const Date& getDate() const;
    TeamId getHomeId() const;
    TeamId getAwayId() const;
    int getHomeGoals() const;
    int getAwayGoals() const;
    int getMatchweek() const;

private:
    MatchId matchId;
    LeagueId leagueId;
    Date date;
    TeamId homeId;
    TeamId awayId;
    int homeGoals;
    int awayGoals;
    int matchweek;
};