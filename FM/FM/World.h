#pragma once

#include <optional>
#include <unordered_map>

#include "League.h"
#include "LeagueContext.h"
#include "LeagueRules.h"
#include "SeasonPlan.h"
#include "Types.h"

class DomainEventPublisher;

class World {
private:
	std::unordered_map<LeagueId, LeagueContext> leagueContexts;
	std::optional<LeagueId> primaryLeagueId;

public: 
	LeagueContext& addLeagueContext(League league, LeagueRules rules, SeasonPlan seasonPlan, DomainEventPublisher& publisher);

	LeagueContext* findLeagueContext(LeagueId leagueId);
	const LeagueContext* findLeagueContext(LeagueId leagueId) const;

	League* findLeagueById(LeagueId leagueId);
	const League* findLeagueById(LeagueId leagueId) const;

	bool hasPrimaryLeagueContext() const;

	LeagueContext& getPrimaryLeagueContext();
	const LeagueContext& getPrimaryLeagueContext() const;
};