#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/match_engine/decision/ActionPlan.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/roster/FormationSlotRole.h"

#include<vector>

struct OffsidePlayerAtRelease {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    FormationSlotRole role = FormationSlotRole::Unknown;
    double playerProgress = 0.0;
    double distanceBeyondLine = 0.0;
};

struct OffsideLineResult {
    bool valid = false;
    double lineProgress = 0.0;
    PlayerId secondLastDefenderId = 0;
    double secondLastDefenderProgress = 0.0;
};

struct OffsideAwarenessTuning {
    double eliteCheckIntervalSeconds = 0.45;
    double poorCheckIntervalSeconds = 1.15;
    double lineBufferMeters = 0.65;
    double safeMarginMeters = 0.45;
    double minimumAdjustmentChance = 0.34;
    double maximumAdjustmentChance = 0.88;
};

struct OffsideAwarenessRequest {
    PlayerId playerId = 0;
    FormationSlotRole role = FormationSlotRole::Unknown;
    PlayerAttributes attributes;
    PitchPoint currentPosition;
    PitchPoint targetPoint;
    PitchPoint ballPosition;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    OffsideLineResult line;
    int currentSecond = 0;
};

struct OffsideAwarenessResult {
    PitchPoint targetPoint;
    bool checked = false;
    bool adjusted = false;
    bool failedToAdjust = false;
    double checkIntervalSeconds = 1.0;
    double distanceToOffsideLine = 0.0;
};
