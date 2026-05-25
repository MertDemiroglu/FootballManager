#pragma once

#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<vector>

struct BallCarrierDecisionModelContext {
    const MatchTeamSnapshot* teamSnapshot = nullptr;
    const MatchTeamSnapshot* opponentSnapshot = nullptr;
    const TeamSimState* teamState = nullptr;
    const TeamSimState* opponentState = nullptr;
    const PlayerSimState* carrierState = nullptr;
    FormationSlotRole carrierRole = FormationSlotRole::Unknown;
    TacticalSetup tacticalSetup;
    PitchPoint ballPosition;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    double carrierPressure = 0.0;
    int actionDepth = 0;
    bool isTransition = false;
};

class BallCarrierDecisionModel {
public:
    std::vector<ActionCandidate> evaluate(const BallCarrierDecisionModelContext& context) const;
};
