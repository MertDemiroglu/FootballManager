#pragma once

#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/offball/OffBallSupportEvent.h"
#include"fm/match_engine/phase/PlayerGameContext.h"

struct OffBallTargetResolveRequest {
    OffBallSupportEvent event;
    PlayerSimState player;
    PlayerGameContext playerContext;
    std::vector<PlayerSimState> teammates;
    std::vector<PlayerSimState> opponents;
    PitchPoint ballPosition;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
};

class OffBallTargetResolver {
public:
    PitchPoint resolve(const OffBallTargetResolveRequest& request) const;
};
