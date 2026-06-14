#pragma once

#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/offball/OffBallSupportEvent.h"
#include"fm/match_engine/offside/OffsideAwarenessModel.h"
#include"fm/match_engine/offside/OffsideLineModel.h"
#include"fm/match_engine/phase/PlayerGameContext.h"

struct OffBallTargetResolveRequest {
    OffBallSupportEvent event;
    PlayerSimState player;
    PlayerGameContext playerContext;
    std::vector<PlayerSimState> teammates;
    std::vector<PlayerSimState> opponents;
    PitchPoint ballPosition;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    PlayerAttributes attributes;
    OffsideLineResult offsideLine;
    int currentSecond = 0;
};

struct OffBallTargetResolveResult {
    PitchPoint targetPoint;
    OffsideAwarenessResult offsideAwareness;
};

class OffBallTargetResolver {
public:
    OffBallTargetResolveResult resolve(const OffBallTargetResolveRequest& request) const;
};
