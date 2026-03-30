#include"LeagueProjection.h"

#include<stdexcept>

LeagueProjection::LeagueProjection(League& league): league(league) {}

void LeagueProjection::onMatchPlayed(const MatchPlayedEvent& event) {
	if (event.leagueId != league.getId()) {
		throw std::runtime_error("league not found for MatchPlayedEvent");
	}
	league.applyMatchPlayedEvent(event);
}


