#pragma once

#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/decision/DecisionContext.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/phase/PlayerGameContext.h"
#include"fm/match_engine/phase/TeamGameContext.h"

#include<vector>

struct BallCarrierDecisionModelContext {
    const MatchTeamSnapshot* teamSnapshot = nullptr;
    const MatchTeamSnapshot* opponentSnapshot = nullptr;
    const TeamSimState* teamState = nullptr;
    const TeamSimState* opponentState = nullptr;
    const PlayerSimState* carrierState = nullptr;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    PlayerDecisionContext playerContext;
    MatchTeamPhase teamPhase = MatchTeamPhase::BuildUp;
    const TeamGameContext* teamGameContext = nullptr;
    const PlayerGameContext* carrierGameContext = nullptr;
};

class BallCarrierDecisionModel {
public:
    std::vector<ActionCandidate> evaluate(const BallCarrierDecisionModelContext& context) const;
};
