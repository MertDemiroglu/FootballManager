#pragma once

#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/decision/BallCarrierDecisionTypes.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<vector>

struct CarryOption {
    CarryOptionKind kind = CarryOptionKind::SafeCarry;
    BallCarrierActionType actionType = BallCarrierActionType::Carry;
    PitchPoint targetPoint;
    double score = 0.0;
    double spaceScore = 0.0;
    double progressionScore = 0.0;
    double controlDifficulty = 0.0;
    double pressureRisk = 0.0;
    double zoneLimitRisk = 0.0;
};

struct CarryOptionEvaluationContext {
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

class CarryOptionEvaluator {
public:
    std::vector<CarryOption> evaluate(const CarryOptionEvaluationContext& context) const;
};
