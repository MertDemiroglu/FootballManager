#pragma once

#include"fm/match_engine/geometry/PitchGeometry.h"

enum class BallTrajectoryType {
    GroundPass,
    ThroughBall,
    LowCross,
    HighCross,
    Cutback,
    Shot,
    Clearance,
    Deflection
};

enum class BallFlightProfile {
    Ground,
    Low,
    Medium,
    High,
    Lofted,
    Shot
};

struct BallTrajectory {
    PitchPoint start;
    PitchPoint intendedTarget;
    PitchPoint actualTarget;
    double startSecond = 0.0;
    double arrivalSecond = 0.0;
    double speedMetersPerSecond = 0.0;
    BallTrajectoryType type = BallTrajectoryType::GroundPass;
    BallFlightProfile flightProfile = BallFlightProfile::Ground;
    double apexHeightMeters = 0.0;
};
