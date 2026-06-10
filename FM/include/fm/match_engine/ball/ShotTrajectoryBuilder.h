#pragma once

#include"fm/match_engine/ball/BallTrajectoryBuilder.h"
#include"fm/match_engine/ball/ShotTypes.h"

struct ShotTrajectoryBuildRequest {
    ShotContext context;
    ShotTargetSelectionResult intendedTarget;
    ShotExecutionResult execution;
    double startSecond = 0.0;
};

class ShotTrajectoryBuilder {
public:
    BallTrajectory build(const ShotTrajectoryBuildRequest& request) const;
};
