#pragma once

#include"League.h"
#include"MatchPlayedEvent.h"

class Game;

class LeagueProjection {
public:
	explicit LeagueProjection(Game& game);
	void onMatchPlayed(const MatchPlayedEvent& event);
private:
	Game& game;
};