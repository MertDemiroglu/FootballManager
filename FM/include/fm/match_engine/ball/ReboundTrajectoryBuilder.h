#pragma once

#include"fm/match_engine/ball/BallTrajectoryBuilder.h"
#include"fm/match_engine/ball/ShotOutcomeResolver.h"

struct ReboundTrajectoryRequest {
    PitchPoint contactPoint;
    BallTrajectory incomingTrajectory;
    ShotExecutionResult execution;
    ShotOutcomeKind outcome = ShotOutcomeKind::SavedRebound;
    double goalkeeperHandling = 50.0;
    double startSecond = 0.0;
    std::uint64_t seed = 0;
};

class ReboundTrajectoryBuilder {
public:
    BallTrajectory build(const ReboundTrajectoryRequest& request) const;
};
