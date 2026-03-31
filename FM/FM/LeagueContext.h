#pragma once

#include "DomainEventPublisher.h"
#include "League.h"
#include "LeagueProjection.h"
#include "LeagueRules.h"
#include "PlayMatchCommandHandler.h"
#include "SeasonPlan.h"
#include "TransferRoom.h"

class LeagueContext {
private:
    League league;
    LeagueRules rules;
    SeasonPlan seasonPlan;
    TransferRoom transferRoom;
    LeagueProjection leagueProjection;
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

    TransferRoom& getTransferRoom();
    const TransferRoom& getTransferRoom() const;

    LeagueProjection& getLeagueProjection();
    const LeagueProjection& getLeagueProjection() const;

    PlayMatchCommandHandler& getPlayMatchCommandHandler();
    const PlayMatchCommandHandler& getPlayMatchCommandHandler() const;

    int getLastSeasonRolloverYear() const;
    void setLastSeasonRolloverYear(int year);


};