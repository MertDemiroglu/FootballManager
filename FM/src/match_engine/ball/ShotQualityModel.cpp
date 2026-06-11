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
    const ShootingModelTuning modelTuning;
    const ShotQualityTuning& tuning = modelTuning.quality;
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
    return std::max(tuning.openPlayXGMinimum, xg);
}

ShotQualityResult ShotQualityModel::evaluate(
    const ShotContext& context,
    ShotType shotType,
    const ShotExecutionResult& execution) const {
    const double baseXG = calculateOpenPlayXG(
        context.shotOrigin,
        context.attackingDirection,
        0.0);
    const ShootingModelTuning modelTuning;
    const ShotQualityTuning& tuning = modelTuning.quality;
    const double pressureFactor = clampDouble(context.pressure / tuning.pressureScale, 0.0, 1.0);
    const double lanePressureFactor =
        clampDouble(context.lanePressure / tuning.lanePressureScale, 0.0, 1.0);
    const double tightAngleFactor =
        clampDouble(1.0 - (context.centrality * tuning.tightAngleCentralityScale), 0.0, 1.0);

    double typePenalty = 0.0;
    if (shotType == ShotType::LongShot) {
        typePenalty = tuning.longShotTypePenalty;
    } else if (shotType == ShotType::TightAngleShot) {
        typePenalty = tuning.tightAngleShotTypePenalty;
    } else if (shotType == ShotType::DesperationShot) {
        typePenalty = tuning.desperationShotTypePenalty;
    } else if (shotType == ShotType::PowerShot) {
        typePenalty = tuning.powerShotTypePenalty;
    }

    const double blockRisk = clampDouble(
        tuning.blockRiskBase
            + lanePressureFactor * tuning.blockRiskLanePressureWeight
            + pressureFactor * tuning.blockRiskPressureWeight
            + clampDouble(
                (tuning.nearestDefenderDistanceReference - context.nearestDefenderDistance)
                    / tuning.nearestDefenderDistanceScale,
                0.0,
                tuning.nearestDefenderRiskMaximum)
            - ((execution.shotPower - tuning.blockRiskPowerBaseline) * tuning.blockRiskPowerReduction),
        tuning.blockRiskMinimum,
        tuning.blockRiskMaximum);
    const double adjustedXG = std::max(
        tuning.adjustedXGMinimum,
        baseXG
            * (1.0 - pressureFactor * tuning.adjustedXGPressurePenalty)
            * (1.0 - lanePressureFactor * tuning.adjustedXGLanePressurePenalty)
            * (1.0 - typePenalty)
            * (1.0 - tightAngleFactor * tuning.adjustedXGTightAnglePenalty));

    const double onTargetDifficulty = clampDouble(
        tuning.onTargetDifficultyBase
            + context.distanceMeters * tuning.onTargetDifficultyDistanceWeight
            + context.pressure * tuning.onTargetDifficultyPressureWeight
            + tightAngleFactor * tuning.onTargetDifficultyTightAngleWeight
            + typePenalty * tuning.onTargetDifficultyTypeWeight
            + execution.targetDeviationMeters * tuning.onTargetDifficultyDeviationWeight
            + execution.heightError * tuning.onTargetDifficultyHeightErrorWeight,
        0.0,
        100.0);
    const double saveDifficulty = clampDouble(
        tuning.saveDifficultyBase
            + adjustedXG * tuning.saveDifficultyAdjustedXGWeight
            + execution.placementQuality * tuning.saveDifficultyPlacementWeight
            + (execution.shotPower - tuning.saveDifficultyPowerBaseline) * tuning.saveDifficultyPowerWeight
            + tightAngleFactor * tuning.saveDifficultyTightAngleWeight,
        0.0,
        100.0);
    const double reboundRisk = clampDouble(
        tuning.reboundRiskBase
            + (execution.shotPower - tuning.reboundRiskPowerBaseline) * tuning.reboundRiskPowerWeight
            + (1.0 - context.goalkeeperStrength / 100.0) * tuning.reboundRiskGoalkeeperWeaknessWeight
            + pressureFactor * tuning.reboundRiskPressureWeight,
        tuning.reboundRiskMinimum,
        tuning.reboundRiskMaximum);

    return ShotQualityResult{
        baseXG,
        adjustedXG,
        blockRisk,
        onTargetDifficulty,
        saveDifficulty,
        reboundRisk
    };
}
