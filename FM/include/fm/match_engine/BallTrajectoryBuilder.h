#pragma once

#include"fm/match_engine/MatchEngineTypes.h"

#include<cstdint>
#include<vector>

struct BallTrajectoryBuildRequest {
    PitchPoint start;
    PitchPoint intendedTarget;
    BallTrajectoryType type = BallTrajectoryType::GroundPass;
    double startSecond = 0.0;

    // 0-100 execution abstraction. Later this can come from passing,
    // crossing, technique, composure, and pressure context.
    double executionQuality = 70.0;
    double pressure = 0.0;

    // Deterministic input for target error. Zero falls back to trajectory data.
    std::uint64_t seed = 0;
};

struct BallTrajectoryBuildResult {
    BallTrajectory trajectory;
    double targetErrorMeters = 0.0;
};

struct DeflectedBallTrajectoryRequest {
    PitchPoint contactPoint;
    PitchPoint incomingStart;
    PitchPoint incomingTarget;
    double deflectionStrength = 0.5;
    double startSecond = 0.0;
    std::uint64_t seed = 0;
};

struct BallTrajectorySample {
    PitchPoint point;
    double second = 0.0;
    double progress = 0.0;
};

class BallTrajectoryBuilder {
public:
    BallTrajectoryBuildResult build(const BallTrajectoryBuildRequest& request) const;
    BallTrajectory buildDeflectedTrajectory(
        const DeflectedBallTrajectoryRequest& request) const;
};

std::vector<BallTrajectorySample> sampleTrajectory(
    const BallTrajectory& trajectory,
    int sampleCount);

double ballHeightAtProgress(const BallTrajectory& trajectory, double progress);
double ballHeightAtSecond(const BallTrajectory& trajectory, double second);
