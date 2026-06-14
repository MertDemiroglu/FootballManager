#include"fm/match_engine/ball/BallTrajectoryBuilder.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<cstdint>
#include<limits>

namespace {
    constexpr double MinBallSpeedMetersPerSecond = 1.0;

    double speedFor(BallTrajectoryType type) {
        switch (type) {
        case BallTrajectoryType::GroundPass:
            return 15.0;
        case BallTrajectoryType::ThroughBall:
            return 17.0;
        case BallTrajectoryType::LowCross:
            return 19.0;
        case BallTrajectoryType::HighCross:
            return 13.0;
        case BallTrajectoryType::Cutback:
            return 14.0;
        case BallTrajectoryType::Shot:
            return 25.0;
        case BallTrajectoryType::Clearance:
            return 22.0;
        case BallTrajectoryType::Deflection:
            return 12.0;
        case BallTrajectoryType::Rebound:
            return 10.0;
        }

        return 15.0;
    }

    double baseErrorMetersFor(BallTrajectoryType type) {
        switch (type) {
        case BallTrajectoryType::GroundPass:
            return 3.0;
        case BallTrajectoryType::ThroughBall:
            return 5.0;
        case BallTrajectoryType::LowCross:
            return 6.0;
        case BallTrajectoryType::HighCross:
            return 9.0;
        case BallTrajectoryType::Cutback:
            return 3.0;
        case BallTrajectoryType::Shot:
            return 5.0;
        case BallTrajectoryType::Clearance:
            return 12.0;
        case BallTrajectoryType::Deflection:
            return 4.0;
        case BallTrajectoryType::Rebound:
            return 3.5;
        }

        return 5.0;
    }

    BallFlightProfile flightProfileFor(BallTrajectoryType type) {
        switch (type) {
        case BallTrajectoryType::GroundPass:
            return BallFlightProfile::Ground;
        case BallTrajectoryType::ThroughBall:
        case BallTrajectoryType::LowCross:
        case BallTrajectoryType::Cutback:
            return BallFlightProfile::Low;
        case BallTrajectoryType::HighCross:
            return BallFlightProfile::High;
        case BallTrajectoryType::Shot:
            return BallFlightProfile::Shot;
        case BallTrajectoryType::Clearance:
            return BallFlightProfile::Lofted;
        case BallTrajectoryType::Deflection:
            return BallFlightProfile::Medium;
        case BallTrajectoryType::Rebound:
            return BallFlightProfile::Low;
        }

        return BallFlightProfile::Ground;
    }

    double apexHeightMetersFor(BallTrajectoryType type) {
        switch (type) {
        case BallTrajectoryType::GroundPass:
            return 0.05;
        case BallTrajectoryType::ThroughBall:
            return 0.4;
        case BallTrajectoryType::LowCross:
            return 0.8;
        case BallTrajectoryType::HighCross:
            return 3.5;
        case BallTrajectoryType::Cutback:
            return 0.2;
        case BallTrajectoryType::Shot:
            return 1.2;
        case BallTrajectoryType::Clearance:
            return 7.0;
        case BallTrajectoryType::Deflection:
            return 1.5;
        case BallTrajectoryType::Rebound:
            return 0.8;
        }

        return 0.05;
    }

    std::uint64_t mix(std::uint64_t value) {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }

    std::uint64_t scaledCoordinateSeed(PitchPoint point) {
        const auto x = static_cast<std::uint64_t>(std::llround(point.x * 1000.0));
        const auto y = static_cast<std::uint64_t>(std::llround(point.y * 1000.0));
        return (x << 32) ^ y;
    }

    std::uint64_t fallbackSeedFor(
        PitchPoint start,
        PitchPoint intendedTarget,
        BallTrajectoryType type) {
        std::uint64_t seed = scaledCoordinateSeed(start);
        seed ^= mix(scaledCoordinateSeed(intendedTarget));
        seed ^= mix(static_cast<std::uint64_t>(type) + 1ULL);
        return seed == 0 ? 0x4d595df4d0f33173ULL : seed;
    }

    double unitFrom(std::uint64_t value) {
        constexpr double denominator =
            static_cast<double>(std::numeric_limits<std::uint64_t>::max());
        return static_cast<double>(value) / denominator;
    }

    PitchPoint applyTargetError(
        PitchPoint intendedTarget,
        BallTrajectoryType type,
        double executionQuality,
        double pressure,
        std::uint64_t seed) {
        const double quality = std::clamp(executionQuality, 0.0, 100.0);
        const double pressureValue = std::clamp(pressure, 0.0, 100.0);
        const double baseErrorMeters = baseErrorMetersFor(type);
        const double qualityPenalty = 1.0 - (quality / 100.0);
        const double pressurePenalty = pressureValue / 100.0;
        const double errorBudgetMeters =
            (baseErrorMeters * qualityPenalty)
            + (baseErrorMeters * 0.45 * pressurePenalty)
            + (2.0 * pressurePenalty);

        const double angleUnit = unitFrom(mix(seed));
        const double magnitudeUnit = unitFrom(mix(seed ^ 0x517cc1b727220a95ULL));
        constexpr double TwoPi = 6.28318530717958647692;

        const double targetErrorMeters = errorBudgetMeters * magnitudeUnit;
        return PitchGeometry::clampToPitch(PitchPoint{
            intendedTarget.x + std::cos(angleUnit * TwoPi) * targetErrorMeters,
            intendedTarget.y + std::sin(angleUnit * TwoPi) * targetErrorMeters
        });
    }
}

BallTrajectoryBuildResult BallTrajectoryBuilder::build(
    const BallTrajectoryBuildRequest& request) const {
    BallTrajectoryBuildResult result;

    const PitchPoint start = PitchGeometry::clampToPitch(request.start);
    const PitchPoint intendedTarget = PitchGeometry::clampToPitch(request.intendedTarget);
    const std::uint64_t seed = request.seed == 0
        ? fallbackSeedFor(start, intendedTarget, request.type)
        : request.seed;

    const PitchPoint actualTarget = applyTargetError(
        intendedTarget,
        request.type,
        request.executionQuality,
        request.pressure,
        seed);

    const double distanceMeters = PitchGeometry::distance(start, actualTarget);
    const double speedMetersPerSecond =
        std::max(speedFor(request.type), MinBallSpeedMetersPerSecond);

    result.trajectory.start = start;
    result.trajectory.intendedTarget = intendedTarget;
    result.trajectory.actualTarget = actualTarget;
    result.trajectory.startSecond = request.startSecond;
    result.trajectory.speedMetersPerSecond = speedMetersPerSecond;
    result.trajectory.arrivalSecond =
        request.startSecond + (distanceMeters / speedMetersPerSecond);
    result.trajectory.type = request.type;
    result.trajectory.flightProfile = flightProfileFor(request.type);
    result.trajectory.apexHeightMeters = apexHeightMetersFor(request.type);
    result.targetErrorMeters = PitchGeometry::distance(intendedTarget, actualTarget);

    return result;
}

BallTrajectory BallTrajectoryBuilder::buildDeflectedTrajectory(
    const DeflectedBallTrajectoryRequest& request) const {
    const PitchPoint contactPoint = PitchGeometry::clampToPitch(request.contactPoint);
    const PitchPoint incomingStart = PitchGeometry::clampToPitch(request.incomingStart);
    const PitchPoint incomingTarget = PitchGeometry::clampToPitch(request.incomingTarget);
    const std::uint64_t seed = request.seed == 0
        ? fallbackSeedFor(incomingStart, incomingTarget, BallTrajectoryType::Deflection)
        : request.seed;

    const double incomingDx = incomingTarget.x - incomingStart.x;
    const double incomingDy = incomingTarget.y - incomingStart.y;
    double baseAngle = std::atan2(incomingDy, incomingDx);
    if (std::abs(incomingDx) < 0.001 && std::abs(incomingDy) < 0.001) {
        baseAngle = unitFrom(mix(seed)) * 6.28318530717958647692;
    }

    const double strength = std::clamp(request.deflectionStrength, 0.0, 1.0);
    const double deviationUnit = unitFrom(mix(seed ^ 0x8bf6d6f7c2ee9331ULL));
    const double signedDeviation = (deviationUnit * 2.0) - 1.0;
    const double deviationRadians = signedDeviation * (0.35 + (strength * 1.05));
    const double distanceMeters =
        8.0 + (strength * 18.0) + (unitFrom(mix(seed ^ 0x36ae2f64fb7a2d9bULL)) * 8.0);

    const PitchPoint actualTarget = PitchGeometry::clampToPitch(PitchPoint{
        contactPoint.x + std::cos(baseAngle + deviationRadians) * distanceMeters,
        contactPoint.y + std::sin(baseAngle + deviationRadians) * distanceMeters
    });
    const double actualDistance = PitchGeometry::distance(contactPoint, actualTarget);
    const double speedMetersPerSecond =
        std::max(8.0 + (strength * 9.0), MinBallSpeedMetersPerSecond);

    BallTrajectory trajectory;
    trajectory.start = contactPoint;
    trajectory.intendedTarget = actualTarget;
    trajectory.actualTarget = actualTarget;
    trajectory.startSecond = request.startSecond;
    trajectory.arrivalSecond = request.startSecond + (actualDistance / speedMetersPerSecond);
    trajectory.speedMetersPerSecond = speedMetersPerSecond;
    trajectory.type = BallTrajectoryType::Deflection;
    trajectory.flightProfile = strength < 0.45
        ? BallFlightProfile::Low
        : BallFlightProfile::Medium;
    trajectory.apexHeightMeters = strength < 0.45
        ? 0.6 + (strength * 1.35)
        : 1.5 + (strength * 1.5);
    return trajectory;
}

std::vector<BallTrajectorySample> sampleTrajectory(
    const BallTrajectory& trajectory,
    int sampleCount) {
    const int count = std::max(sampleCount, 2);
    std::vector<BallTrajectorySample> samples;
    samples.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        const double progress = static_cast<double>(i) / static_cast<double>(count - 1);
        samples.push_back(BallTrajectorySample{
            PitchPoint{
                trajectory.start.x
                    + ((trajectory.actualTarget.x - trajectory.start.x) * progress),
                trajectory.start.y
                    + ((trajectory.actualTarget.y - trajectory.start.y) * progress)
            },
            trajectory.startSecond
                + ((trajectory.arrivalSecond - trajectory.startSecond) * progress),
            progress
        });
    }

    return samples;
}

double ballHeightAtProgress(const BallTrajectory& trajectory, double progress) {
    constexpr double GroundHeightMeters = 0.05;
    const double clampedProgress = std::clamp(progress, 0.0, 1.0);

    if (trajectory.flightProfile == BallFlightProfile::Ground) {
        return GroundHeightMeters;
    }

    const double apexHeightMeters = std::max(0.0, trajectory.apexHeightMeters);
    const double height =
        GroundHeightMeters
        + (4.0 * apexHeightMeters * clampedProgress * (1.0 - clampedProgress));
    return std::max(0.0, height);
}

double ballHeightAtSecond(const BallTrajectory& trajectory, double second) {
    const double durationSeconds = trajectory.arrivalSecond - trajectory.startSecond;
    if (durationSeconds <= 0.001) {
        return ballHeightAtProgress(trajectory, 1.0);
    }

    return ballHeightAtProgress(
        trajectory,
        (second - trajectory.startSecond) / durationSeconds);
}
