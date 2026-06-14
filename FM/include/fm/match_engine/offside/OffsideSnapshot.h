#pragma once

#include"fm/match_engine/offside/OffsideTypes.h"

struct OffsideSnapshot {
    TeamId passerTeamId = 0;
    PlayerId passerPlayerId = 0;
    BallCarrierActionType actionType = BallCarrierActionType::Hold;
    PitchPoint passStartPoint;
    PitchPoint targetPoint;
    PitchPoint ballPosition;
    double offsideLineProgress = 0.0;
    PlayerId secondLastDefenderId = 0;
    double secondLastDefenderProgress = 0.0;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    std::vector<OffsidePlayerAtRelease> attackersOffsideAtRelease;
};

