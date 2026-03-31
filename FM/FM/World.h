#pragma once

#include<functional>
#include<optional>
#include<unordered_map>

#include"League.h"
#include"LeagueContext.h"
#include"LeagueRules.h"
#include"SeasonPlan.h"
#include"TransferRoom.h"
#include"Types.h"

class DomainEventPublisher;
class Team;

class World {
private:
	std::unordered_map<LeagueId, LeagueContext> leagueContexts;
	std::optional<LeagueId> primaryLeagueId;
	TransferRoom transferRoom;

public: 

	World();

	LeagueContext& addLeagueContext(League league, LeagueRules rules, SeasonPlan seasonPlan, DomainEventPublisher& publisher);

	LeagueContext* findLeagueContext(LeagueId leagueId);
	const LeagueContext* findLeagueContext(LeagueId leagueId) const;

	League* findLeagueById(LeagueId leagueId);
	const League* findLeagueById(LeagueId leagueId) const;

	TransferRoom& getTransferRoom();
	const TransferRoom& getTransferRoom() const;

	Team* findTeamByName(const std::string& teamName, LeagueId* leagueIdOut = nullptr);
	const Team* findTeamByName(const std::string& teamName, LeagueId* leagueIdOut = nullptr) const;

	bool hasPrimaryLeagueContext() const;

	LeagueContext& getPrimaryLeagueContext();
	const LeagueContext& getPrimaryLeagueContext() const;

	//Ligleri gezip islem yapmak icin
	void forEachLeagueContext(const std::function<void(LeagueContext&)>& visitor);
	void forEachLeagueContext(const std::function<void(const LeagueContext&)>& visitor) const;
};