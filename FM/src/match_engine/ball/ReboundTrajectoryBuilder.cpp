#include"fm/match_engine/ball/ReboundTrajectoryBuilder.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<cmath>

namespace {
    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double signedUnit(std::uint64_t seed) {
        return (matchEngineDeterministicUnitInterval(seed) * 2.0) - 1.0;
    }
}

BallTrajectory ReboundTrajectoryBuilder::build(const ReboundTrajectoryRequest& request) const {
    const PitchPoint contactPoint = PitchGeometry::clampToPitch(request.contactPoint);
    const double incomingAngle = std::atan2(
        request.incomingTrajectory.actualTarget.y - request.incomingTrajectory.start.y,
        request.incomingTrajectory.actualTarget.x - request.incomingTrajectory.start.x);
    const double handlingFactor = clampDouble(request.goalkeeperHandling / 100.0, 0.0, 1.0);
    const double powerFactor = clampDouble((request.execution.shotPower - 18.0) / 18.0, 0.0, 1.0);
    const bool blocked = request.outcome == ShotOutcomeKind::Blocked;
    const double baseDistance = blocked
        ? 7.0 + powerFactor * 14.0
        : 5.0 + powerFactor * 12.0 + (1.0 - handlingFactor) * 8.0;
    const double angleSpread = blocked ? 1.25 : 0.95;
    const double angle = incomingAngle
        + 3.14159265358979323846
        + signedUnit(request.seed ^ 0x781ULL) * angleSpread;
    const double distance = baseDistance + matchEngineDeterministicUnitInterval(request.seed ^ 0x515ULL) * 7.0;
    const PitchPoint target = PitchGeometry::clampToPitch(PitchPoint{
        contactPoint.x + std::cos(angle) * distance,
        contactPoint.y + std::sin(angle) * distance
    });

    BallTrajectory trajectory;
    trajectory.start = contactPoint;
    trajectory.intendedTarget = target;
    trajectory.actualTarget = target;
    trajectory.startSecond = request.startSecond;
    trajectory.type = blocked ? BallTrajectoryType::Deflection : BallTrajectoryType::Rebound;
    trajectory.flightProfile = blocked ? BallFlightProfile::Medium : BallFlightProfile::Low;
    trajectory.speedMetersPerSecond = std::max(7.0 + powerFactor * 8.0, 1.0);
    trajectory.arrivalSecond =
        request.startSecond + (PitchGeometry::distance(contactPoint, target) / trajectory.speedMetersPerSecond);
    trajectory.apexHeightMeters = blocked ? 1.4 + powerFactor : 0.45 + powerFactor * 0.9;
    return trajectory;
}
