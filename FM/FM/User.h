#pragma once
#include<string>
#include"Types.h"

class Game;
class User {
private:
	std::string userName = "xx";
	LeagueId managedLeagueId = 0;
	TeamId managedTeamId = 0;
public:
	void setTeam(LeagueId leagueId, TeamId teamId);
	LeagueId getManagedLeagueId() const;
	TeamId getManagedTeamId() const;
};