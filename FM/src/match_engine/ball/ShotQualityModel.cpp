#include"fm/match_engine/ball/ShotQualityModel.h"

#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>

namespace {
    constexpr double Pi = 3.14159265358979323846;

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    PitchPoint goalCenterFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    double goalLineXFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters
            : 0.0;
    }

    double goalAngleRadians(PitchPoint shotLocation, AttackingDirection direction) {
        const double halfGoalWidth = PitchGeometry::GoalWidthMeters / 2.0;
        const double centerY = PitchGeometry::WidthMeters / 2.0;
        const double goalX = goalLineXFor(direction);

        const PitchPoint postA{ goalX, centerY - halfGoalWidth };
        const PitchPoint postB{ goalX, centerY + halfGoalWidth };

        const double ax = postA.x - shotLocation.x;
        const double ay = postA.y - shotLocation.y;
        const double bx = postB.x - shotLocation.x;
        const double by = postB.y - shotLocation.y;

        const double lenA = std::hypot(ax, ay);
        const double lenB = std::hypot(bx, by);
        if (lenA <= 0.001 || lenB <= 0.001) {
            return Pi;
        }

        const double cosine = clampDouble(((ax * bx) + (ay * by)) / (lenA * lenB), -1.0, 1.0);
        return std::acos(cosine);
    }
}

double ShotQualityModel::calculateOpenPlayXG(
    PitchPoint shotLocation,
    AttackingDirection attackingDirection,
    double pressure) {
    const ShootingModelTuning tuning;
    const double distanceMeters =
        PitchGeometry::distance(shotLocation, goalCenterFor(attackingDirection));
    const double angleRadians = goalAngleRadians(shotLocation, attackingDirection);
    const double clampedPressure = clampDouble(pressure, 0.0, 100.0);

    const double logit =
        tuning.openPlayXGIntercept
        - (tuning.openPlayXGDistanceCoefficient * distanceMeters)
        + (tuning.openPlayXGAngleCoefficient * angleRadians)
        - (tuning.openPlayXGPressureCoefficient * clampedPressure);
    const double xg = 1.0 / (1.0 + std::exp(-logit));
    return clampDouble(xg, tuning.openPlayXGMinimum, tuning.openPlayXGMaximum);
}

ShotQualityResult ShotQualityModel::evaluate(
    const ShotContext& context,
    ShotType shotType,
    const ShotExecutionResult& execution) const {
    const double baseXG = calculateOpenPlayXG(
        context.shotOrigin,
        context.attackingDirection,
        0.0);
    const double pressureFactor = clampDouble(context.pressure / 100.0, 0.0, 1.0);
    const double lanePressureFactor = clampDouble(context.lanePressure / 100.0, 0.0, 1.0);
    const double tightAngleFactor = clampDouble(1.0 - (context.centrality * 4.0), 0.0, 1.0);

    double typePenalty = 0.0;
    if (shotType == ShotType::LongShot) {
        typePenalty = 0.10;
    } else if (shotType == ShotType::TightAngleShot) {
        typePenalty = 0.12;
    } else if (shotType == ShotType::DesperationShot) {
        typePenalty = 0.18;
    } else if (shotType == ShotType::PowerShot) {
        typePenalty = 0.04;
    }

    const double blockRisk = clampDouble(
        0.04
            + lanePressureFactor * 0.34
            + pressureFactor * 0.12
            + clampDouble((22.0 - context.nearestDefenderDistance) / 45.0, 0.0, 0.22)
            - ((execution.shotPower - 24.0) * 0.006),
        0.02,
        0.58);
    const double adjustedXG = clampDouble(
        baseXG
            * (1.0 - pressureFactor * 0.30)
            * (1.0 - lanePressureFactor * 0.18)
            * (1.0 - typePenalty)
            * (1.0 - tightAngleFactor * 0.18),
        0.003,
        0.42);

    const double onTargetDifficulty = clampDouble(
        18.0
            + context.distanceMeters * 1.25
            + context.pressure * 0.34
            + tightAngleFactor * 30.0
            + typePenalty * 70.0
            + execution.targetDeviationMeters * 6.0
            + execution.heightError * 7.0,
        0.0,
        100.0);
    const double saveDifficulty = clampDouble(
        34.0
            + adjustedXG * 92.0
            + execution.placementQuality * 0.18
            + (execution.shotPower - 22.0) * 1.4
            + tightAngleFactor * 8.0,
        0.0,
        100.0);
    const double reboundRisk = clampDouble(
        0.18
            + (execution.shotPower - 22.0) * 0.018
            + (1.0 - context.goalkeeperStrength / 100.0) * 0.18
            + pressureFactor * 0.08,
        0.05,
        0.62);

    return ShotQualityResult{
        baseXG,
        adjustedXG,
        blockRisk,
        onTargetDifficulty,
        saveDifficulty,
        reboundRisk
    };
}
