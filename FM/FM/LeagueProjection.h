#pragma once

#include"League.h"
#include"MatchPlayedEvent.h"

class LeagueProjection {
public:
	explicit LeagueProjection(League& league);
	void onMatchPlayed(const MatchPlayedEvent& event);
private:
	League& league;
};