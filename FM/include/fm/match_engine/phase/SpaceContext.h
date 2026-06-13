#pragma once

#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/phase/MatchPhaseTypes.h"

struct SpaceContextSnapshot {
    BallZone ballZone = BallZone::MiddleThird;
    BallFlank ballFlank = BallFlank::Center;
    double ballProgress = 0.0;
    int playersAheadOfBall = 0;
    int playersBehindBall = 0;
    int nearestSupportCount = 0;
    bool supportAvailableNearBall = false;
    double openForwardLaneScore = 0.0;
    double openWideLaneScore = 0.0;
    bool centralSpaceAvailable = false;
    bool wideSpaceAvailableLeft = false;
    bool wideSpaceAvailableRight = false;
    double counterOpportunityScore = 0.0;
    double counterThreatScore = 0.0;
    bool restDefenseStable = false;
};

class SpaceContextModel {
public:
    SpaceContextSnapshot evaluate(
        const TeamSimState& team,
        const TeamSimState& opponent,
        PitchPoint ballPosition) const;
};

