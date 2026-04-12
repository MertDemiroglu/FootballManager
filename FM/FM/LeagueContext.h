#pragma once

#include "DomainEventPublisher.h"
#include "League.h"
#include "LeagueRules.h"
#include "PlayMatchCommandHandler.h"
#include "SeasonPlan.h"

class LeagueContext {
private:
    League league;
    LeagueRules rules;
    SeasonPlan seasonPlan;
    PlayMatchCommandHandler playMatchCommandHandler;
    int lastSeasonRolloverYear = -1;

public:
    LeagueContext(League league, LeagueRules rules, SeasonPlan seasonPlan, DomainEventPublisher& publisher);

    LeagueContext(const LeagueContext&) = delete;
    LeagueContext& operator=(const LeagueContext&) = delete;
    LeagueContext(LeagueContext&&) = delete;
    LeagueContext& operator=(LeagueContext&&) = delete;

    League& getLeague();
    const League& getLeague() const;

    LeagueRules& getRules();
    const LeagueRules& getRules() const;

    SeasonPlan& getSeasonPlan();
    const SeasonPlan& getSeasonPlan() const;

    PlayMatchCommandHandler& getPlayMatchCommandHandler();
    const PlayMatchCommandHandler& getPlayMatchCommandHandler() const;

    int getLastSeasonRolloverYear() const;
    void setLastSeasonRolloverYear(int year);


};
