#pragma once

#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/decision/BallCarrierDecisionTypes.h"
#include"fm/match_engine/decision/DecisionContext.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<vector>

struct ShotOption {
    ShotOptionKind kind = ShotOptionKind::OpenPlayShot;
    BallCarrierActionType actionType = BallCarrierActionType::Shoot;
    PitchPoint targetPoint;
    double score = 0.0;
    double estimatedXG = 0.0;
    double angleScore = 0.0;
    double distanceScore = 0.0;
    double pressurePenalty = 0.0;
    double shooterConfidence = 0.0;
};

struct ShotOptionEvaluationContext {
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
    DecisionMatchPhase phase = DecisionMatchPhase::BuildUp;
    int possessionActionCount = 0;
    double ballProgression = 0.0;
    bool safeCirculationAvailable = false;
};

class ShotDecisionEvaluator {
public:
    std::vector<ShotOption> evaluate(const ShotOptionEvaluationContext& context) const;
};
