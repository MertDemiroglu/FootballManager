#pragma once

#include<functional>
#include<optional>
#include<unordered_map>

#include"DomainEventPublisher.h"
#include"League.h"
#include"LeagueContext.h"
#include"LeagueRules.h"
#include"SeasonPlan.h"
#include"TransferOfferService.h"
#include"TransferRoom.h"
#include"fm/common/Types.h"

class Team;

class World {
private:
	std::unordered_map<LeagueId, LeagueContext> leagueContexts;
	std::optional<LeagueId> primaryLeagueId;
	DomainEventPublisher domainEventPublisher;
	TransferRoom transferRoom;
	TransferOfferService transferOfferService;
	MatchId nextMatchId = 1;

public: 

	World();

	LeagueContext& addLeagueContext(League league, LeagueRules rules, SeasonPlan seasonPlan);

	LeagueContext* findLeagueContext(LeagueId leagueId);
	const LeagueContext* findLeagueContext(LeagueId leagueId) const;

	League* findLeagueById(LeagueId leagueId);
	const League* findLeagueById(LeagueId leagueId) const;

	DomainEventPublisher& getDomainEventPublisher();
	const DomainEventPublisher& getDomainEventPublisher() const;

	TransferRoom& getTransferRoom();
	const TransferRoom& getTransferRoom() const;

	TransferOfferService& getTransferOfferService();
	const TransferOfferService& getTransferOfferService() const;

	MatchId allocateMatchId();

	bool hasPrimaryLeagueContext() const;

	LeagueContext& getPrimaryLeagueContext();
	const LeagueContext& getPrimaryLeagueContext() const;

	//Ligleri gezip islem yapmak icin
	void forEachLeagueContext(const std::function<void(LeagueContext&)>& visitor);
	void forEachLeagueContext(const std::function<void(const LeagueContext&)>& visitor) const;
};