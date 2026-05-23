#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/BallTrajectoryBuilder.h"
#include"fm/match_engine/MatchSimulationState.h"

#include<optional>
#include<vector>

struct InterceptionCandidate {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PitchPoint interceptionPoint;
    double ballArrivalSecond = 0.0;
    double playerArrivalSecond = 0.0;
    double arrivalMarginSeconds = 0.0;
    double qualityScore = 0.0;
};

struct InterceptionResolverRequest {
    BallTrajectory trajectory;
    std::vector<PlayerSimState> defenders;
    int trajectorySampleCount = 7;
    double reactionWindowSeconds = 0.35;
};

struct InterceptionResolverResult {
    std::vector<InterceptionCandidate> candidates;
    std::optional<InterceptionCandidate> bestCandidate;
};

class InterceptionResolver {
public:
    InterceptionResolverResult resolve(const InterceptionResolverRequest& request) const;
};
