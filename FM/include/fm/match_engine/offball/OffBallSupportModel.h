#pragma once

#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/offball/OffBallSupportEvent.h"
#include"fm/match_engine/offball/OffBallTargetResolver.h"
#include"fm/match_engine/offball/RestDefenseModel.h"
#include"fm/match_engine/phase/PlayerGameContext.h"
#include"fm/match_engine/phase/TeamGameContext.h"

#include<vector>

struct OffBallSupportModelRequest {
    const TeamSimState* team = nullptr;
    const TeamSimState* opponent = nullptr;
    const MatchTeamSnapshot* teamSnapshot = nullptr;
    TeamGameContext teamContext;
    std::vector<PlayerGameContext> playerContexts;
    std::vector<OffBallSupportEvent> activeEvents;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    OffsideLineResult offsideLine;
    int currentSecond = 0;
};

struct OffBallSupportModelResult {
    std::vector<OffBallSupportEvent> events;
    int rejectedByRestDefense = 0;
    bool restDefenseStableBeforeSupport = false;
    bool restDefenseStableAfterSupport = false;
};

class OffBallSupportModel {
public:
    OffBallSupportModelResult evaluate(const OffBallSupportModelRequest& request) const;

private:
    OffBallTargetResolver targetResolver_;
    RestDefenseModel restDefenseModel_;
};
