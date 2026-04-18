#pragma once

#include"fm/common/Date.h"
#include"fm/common/Types.h"

struct MatchPlayedEvent {
	MatchId matchId = 0;
	LeagueId leagueId = 0;
	int seasonYear = 0;
	Date date;
	TeamId homeId = 0;
	TeamId awayId = 0;
	int matchweek = 0;
	int homeGoals = 0;
	int awayGoals = 0;
};