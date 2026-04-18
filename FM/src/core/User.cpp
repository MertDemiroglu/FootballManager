#include"fm/core/User.h"

void User::setTeam(LeagueId leagueId, TeamId teamId) {
	managedLeagueId = leagueId;
	managedTeamId = teamId;
}

LeagueId User::getManagedLeagueId() const {
	return managedLeagueId;
}

TeamId User::getManagedTeamId() const {
	return managedTeamId;
}