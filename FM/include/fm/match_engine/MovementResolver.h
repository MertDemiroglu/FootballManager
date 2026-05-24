#pragma once

#include"fm/match_engine/MatchSimulationState.h"

struct MovementResolutionRequest {
    PlayerSimState player;
    PlayerIntent intent;
    PitchPoint target;
    double deltaSeconds = 1.0;
};

struct MovementResolutionResult {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PitchPoint previousPosition;
    PitchPoint targetPosition;
    PitchPoint newPosition;
    double distanceToTargetBefore = 0.0;
    double distanceMoved = 0.0;
    bool reachedTarget = false;
};

class MovementResolver {
public:
    MovementResolutionResult resolve(const MovementResolutionRequest& request) const;
};
