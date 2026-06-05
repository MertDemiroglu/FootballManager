#include"fm/match_engine/ball/ShotTrajectoryBuilder.h"

#include<algorithm>

BallTrajectory ShotTrajectoryBuilder::build(const ShotTrajectoryBuildRequest& request) const {
    BallTrajectory trajectory;
    trajectory.start = PitchGeometry::clampToPitch(request.context.shotOrigin);
    trajectory.intendedTarget = PitchGeometry::clampToPitch(request.intendedTarget.intendedTarget);
    trajectory.actualTarget = PitchGeometry::clampToPitch(request.execution.actualTarget);
    trajectory.startSecond = request.startSecond;
    trajectory.type = BallTrajectoryType::Shot;
    trajectory.flightProfile = BallFlightProfile::Shot;
    trajectory.speedMetersPerSecond = std::max(request.execution.shotPower, 1.0);
    const double distanceMeters = PitchGeometry::distance(trajectory.start, trajectory.actualTarget);
    trajectory.arrivalSecond = request.startSecond + (distanceMeters / trajectory.speedMetersPerSecond);

    if (request.execution.actualZone.height == ShotTargetHeight::Low) {
        trajectory.apexHeightMeters = 0.45;
    } else if (request.execution.actualZone.height == ShotTargetHeight::Mid) {
        trajectory.apexHeightMeters = 1.15;
    } else {
        trajectory.apexHeightMeters = 2.25;
    }
    return trajectory;
}
