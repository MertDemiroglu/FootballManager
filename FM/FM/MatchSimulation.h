#pragma once

#include "Date.h"
#include "League.h"

class Team;

namespace MatchSimulation {
	MatchResult buildStrengthBasedResult(const Team& homeTeam, const Team& awayTeam, const Date& date);
}