#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<vector>

enum class PressureDangerZone {
    Midfield,
    FinalThird,
    Box,
    CentralBox,
    Goalmouth
};

struct PressurePlayer {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PitchPoint position;
    PlayerIntentType intentType = PlayerIntentType::None;
};

struct PressureContext {
    PlayerId closestOutfieldDefenderId = 0;
    TeamId closestOutfieldDefenderTeamId = 0;
    PitchPoint closestOutfieldDefenderPosition;
    double closestOutfieldDefenderDistance = -1.0;
    double defenderAngleToGoalDegrees = 180.0;
    bool defenderBetweenBallAndGoal = false;
    bool defenderBetweenBallAndPath = false;
    int supportDefendersNearby = 0;
    double pressureStrength = 0.0;
    PressureDangerZone dangerZone = PressureDangerZone::Midfield;
};

struct PressureContextRequest {
    std::vector<PressurePlayer> defenders;
    PitchPoint ballPosition;
    PitchPoint intendedTarget;
    PitchPoint opponentGoalCenter;
    AttackingDirection direction = AttackingDirection::HomeToAway;
    BallCarrierActionType actionType = BallCarrierActionType::Carry;
};

class PressureModel {
public:
    PressureContext build(const PressureContextRequest& request) const;
};

double zonePressureBonus(PressureDangerZone zone);
double contestDistanceLimit(PressureDangerZone zone, BallCarrierActionType actionType);
bool isRealContest(const PressureContext& pressure, BallCarrierActionType actionType);
