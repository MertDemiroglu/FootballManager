#pragma once

#include"Date.h"
#include"Types.h"

struct MatchPlayedEvent {
	int seasonYear = 0;
	Date date;
	TeamId homeId = 0;
	TeamId awayId = 0;
	int matchWeek = 0;
	int homeGoals = 0;
	int awayGoals = 0;
};