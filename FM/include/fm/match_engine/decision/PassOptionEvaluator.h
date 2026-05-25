#pragma once

#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/decision/BallCarrierDecisionTypes.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<vector>

struct PassOption {
    PassOptionKind kind = PassOptionKind::SafePass;
    BallCarrierActionType actionType = BallCarrierActionType::ShortPass;
    PlayerId targetPlayerId = 0;
    PitchPoint targetPoint;
    double score = 0.0;
    double safetyScore = 0.0;
    double progressionScore = 0.0;
    double laneRisk = 0.0;
    double receiverPressure = 0.0;
    double executionDifficulty = 0.0;
};

struct PassOptionEvaluationContext {
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
};

class PassOptionEvaluator {
public:
    std::vector<PassOption> evaluate(const PassOptionEvaluationContext& context) const;
};
