#include"fm/roster/Contract.h"

Contract::Contract(PlayerId playerId, TeamId teamId, Money wage, int years) : playerId(playerId), teamId(teamId), wage(wage), yearsRemaining(years){}

Money Contract::getWage() const {
	return wage;
}

void Contract::advanceYear() {
	--yearsRemaining;
}

bool Contract::isExpired() const {
	return yearsRemaining == 0;
}

PlayerId Contract::getPlayerId() const {
	return playerId;
}

TeamId Contract::getTeamId() const {
	return teamId;
}

void Contract::setTeamId(TeamId newTeamId) {
	teamId = newTeamId;
}
