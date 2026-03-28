#include"LeagueProjection.h"
#include"Game.h"

#include<stdexcept>

LeagueProjection::LeagueProjection(Game& game) : game(game){}

void LeagueProjection::onMatchPlayed(const MatchPlayedEvent& event) {
	League* league = game.findLeagueById(event.leagueId);
	if (!league) {
		throw std::runtime_error("league not found for MatchPlayedEvent");
	}
	league->applyMatchPlayedEvent(event);
}


