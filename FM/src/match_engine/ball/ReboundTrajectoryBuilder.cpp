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
    const ShootingModelTuning modelTuning;
    const ReboundModelTuning& tuning = modelTuning.rebound;
    const PitchPoint contactPoint = PitchGeometry::clampToPitch(request.contactPoint);
    const double incomingAngle = std::atan2(
        request.incomingTrajectory.actualTarget.y - request.incomingTrajectory.start.y,
        request.incomingTrajectory.actualTarget.x - request.incomingTrajectory.start.x);
    const double handlingFactor = clampDouble(request.goalkeeperHandling / 100.0, 0.0, 1.0);
    const double powerFactor = clampDouble(
        (request.execution.shotPower - tuning.shotPowerBaseline) / tuning.shotPowerScale,
        0.0,
        1.0);
    const double strengthFactor = clampDouble(request.deflectionStrength, 0.0, 1.0);
    const bool blocked = request.outcome == ShotOutcomeKind::Blocked;
    const double baseDistance = blocked
        ? tuning.blockedDeflectionBaseDistance
            + powerFactor * tuning.blockedDeflectionPowerDistanceScale
            + strengthFactor * tuning.deflectionStrengthDistanceScale
        : tuning.savedReboundBaseDistance
            + powerFactor * tuning.savedReboundPowerDistanceScale
            + (1.0 - handlingFactor) * tuning.handlingDistanceReductionScale;
    const double lateralSide = signedUnit(request.seed ^ 0x4a7ULL) >= 0.0 ? 1.0 : -1.0;
    const double savedReboundBaseAngle =
        incomingAngle
        + lateralSide * tuning.savedReboundLateralAngleRadians
        + tuning.reboundBackwardAngleRadians * tuning.savedReboundBackwardBlend;
    const double blockedDeflectionBaseAngle = incomingAngle + tuning.reboundBackwardAngleRadians;
    const double angleSpread = blocked
        ? tuning.blockedDeflectionAngleSpread
        : tuning.savedReboundAngleSpread;
    const double angle = (blocked ? blockedDeflectionBaseAngle : savedReboundBaseAngle)
        + signedUnit(request.seed ^ 0x781ULL) * angleSpread;
    const double distance =
        baseDistance
        + matchEngineDeterministicUnitInterval(request.seed ^ 0x515ULL) * tuning.randomDistanceScale;
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
    trajectory.speedMetersPerSecond = std::max(
        tuning.reboundSpeedBase
            + powerFactor * tuning.reboundSpeedPowerScale
            + (blocked ? strengthFactor * tuning.deflectionStrengthSpeedScale : 0.0),
        tuning.minimumReboundSpeed);
    trajectory.arrivalSecond =
        request.startSecond + (PitchGeometry::distance(contactPoint, target) / trajectory.speedMetersPerSecond);
    trajectory.apexHeightMeters = blocked
        ? tuning.blockedDeflectionApexBase + powerFactor * tuning.blockedDeflectionApexPowerScale
        : tuning.savedReboundApexBase + powerFactor * tuning.savedReboundApexPowerScale;
    return trajectory;
}
