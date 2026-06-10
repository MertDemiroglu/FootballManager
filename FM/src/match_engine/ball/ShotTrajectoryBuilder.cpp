#include"fm/match_engine/ball/ShotTrajectoryBuilder.h"

#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>

BallTrajectory ShotTrajectoryBuilder::build(const ShotTrajectoryBuildRequest& request) const {
    const ShootingModelTuning modelTuning;
    const ShotTrajectoryTuning& tuning = modelTuning.trajectory;
    BallTrajectory trajectory;
    trajectory.start = PitchGeometry::clampToPitch(request.context.shotOrigin);
    trajectory.intendedTarget = PitchGeometry::clampToPitch(request.intendedTarget.intendedTarget);
    trajectory.actualTarget = PitchGeometry::clampToPitch(request.execution.actualTarget);
    trajectory.startSecond = request.startSecond;
    trajectory.type = BallTrajectoryType::Shot;
    trajectory.flightProfile = BallFlightProfile::Shot;
    trajectory.speedMetersPerSecond =
        std::max(request.execution.shotPower, tuning.minimumShotSpeedMetersPerSecond);
    const double distanceMeters = PitchGeometry::distance(trajectory.start, trajectory.actualTarget);
    trajectory.arrivalSecond = request.startSecond + (distanceMeters / trajectory.speedMetersPerSecond);

    if (request.execution.actualZone.height == ShotTargetHeight::Low) {
        trajectory.apexHeightMeters = tuning.lowShotApexMeters;
    } else if (request.execution.actualZone.height == ShotTargetHeight::Mid) {
        trajectory.apexHeightMeters = tuning.midShotApexMeters;
    } else {
        trajectory.apexHeightMeters = tuning.highShotApexMeters;
    }
    return trajectory;
}
