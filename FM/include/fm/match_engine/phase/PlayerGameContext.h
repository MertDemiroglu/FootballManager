#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/roster/FormationSlotRole.h"

struct PlayerGameContext {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    FormationSlotRole role = FormationSlotRole::Unknown;
    PitchPoint currentPosition;
    PitchPoint assignedPosition;
    double distanceToBall = 0.0;
    bool sameFlankAsBall = false;
    bool isBallSideFullback = false;
    bool isFarSideFullback = false;
    bool isBallSideWinger = false;
    bool isFarSideWinger = false;
    bool isCentralMidfielder = false;
    bool isStriker = false;
    bool inPrimaryZone = false;
    bool inExtraZone = false;
    bool aheadOfBall = false;
    bool behindBall = false;
    bool canSupportBallCarrier = false;
    bool canJoinAttackWithoutBreakingRestDefense = false;
    bool shouldHoldRestDefense = false;
    int nearbyOpponents = 0;
    int nearbyTeammates = 0;
    bool nearestPassingLaneAvailable = false;
};

