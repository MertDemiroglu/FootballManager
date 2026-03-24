#include"LeagueProjection.h"

#include<stdexcept>

LeagueProjection::LeagueProjection(League& league) : league(league){}

void LeagueProjection::onMatchPlayed(const MatchPlayedEvent& event) {
	FixtureMatch* match = league.findFixtureMatch(event.date, event.homeId, event.awayId);
	if (!match) {
		throw std::runtime_error("fixture match not found for MatchPlayedEvent");
	}

	if (match->played) {
		return;
	}

	if (match->matchweek != event.matchWeek) {
		throw std::logic_error("match played event matchweek mismatch");
	}
	league.applyMatchResult(event.date, event.homeId, event.awayId, MatchResult{ event.homeGoals, event.awayGoals });
}