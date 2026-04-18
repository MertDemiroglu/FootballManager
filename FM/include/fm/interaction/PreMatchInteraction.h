#pragma once

#include"fm/common/Date.h"
#include"fm/interaction/GameInteraction.h"
#include"TeamSheet.h"
#include"fm/common/Types.h"

class PreMatchInteraction : public GameInteraction {
private:
    MatchId matchId;
    LeagueId leagueId;
    Date date;
    TeamId homeId;
    TeamId awayId;
    int matchweek;
    TeamSheet homeSheet;
    TeamSheet awaySheet;

public:
	PreMatchInteraction(MatchId matchId,
        LeagueId leagueId,
        const Date& date,
        TeamId homeId,
        TeamId awayId,
        int matchweek,
        TeamSheet homeSheet,
        TeamSheet awaySheet);

    MatchId getMatchId() const;
    LeagueId getLeagueId() const;
    const Date& getDate() const;
    TeamId getHomeId() const;
    TeamId getAwayId() const;
    int getMatchweek() const;
    const TeamSheet& getHomeSheet() const;
    const TeamSheet& getAwaySheet() const;
};
