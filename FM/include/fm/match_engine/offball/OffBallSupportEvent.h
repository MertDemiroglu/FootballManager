#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/offball/OffBallEventTypes.h"
#include"fm/match_engine/offball/SupportRegion.h"
#include"fm/match_engine/phase/MatchPhaseTypes.h"

#include<string>

struct OffBallSupportEvent {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    OffBallEventType eventType = OffBallEventType::None;
    MatchTeamPhase sourcePhase = MatchTeamPhase::BuildUp;
    std::string sourceReason;
    int startSecond = 0;
    double maxDurationSeconds = 6.0;
    SupportRegion targetRegion;
    PitchPoint startPosition;
    PitchPoint resolvedTargetPoint;
    double distanceToTargetAtCreation = 0.0;
    double distanceToTargetAtCompletion = 0.0;
    double distanceMovedDuringEvent = 0.0;
    double priority = 0.0;
    bool requiresRestDefenseSafety = false;
    bool expiresOnPossessionLoss = true;
    bool expiresOnShot = true;
    bool expiresOnBallOut = true;
    bool expiresOnOpponentControl = true;
    bool expiresOnPhaseChange = true;
    bool completed = false;
    OffBallEventCompletionReason completionReason = OffBallEventCompletionReason::None;
    int completedSecond = 0;
    bool offsideAwarenessChecked = false;
    bool offsideAwarenessAdjusted = false;
    bool offsideAwarenessFailedToAdjust = false;
    double offsideAwarenessCheckInterval = 0.0;
    double distanceToOffsideLineAtTarget = 0.0;
};
