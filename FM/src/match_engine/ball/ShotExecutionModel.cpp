#include"fm/match_engine/ball/ShotExecutionModel.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/ball/ShotQualityModel.h"
#include"fm/match_engine/ball/ShotTargetSelector.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<cmath>

namespace {
    double attributeOrDefault(int value) {
        const ShootingModelTuning tuning;
        return value > 0 ? std::clamp(static_cast<double>(value), 0.0, 100.0) : tuning.defaultAttribute;
    }

    double typeDifficulty(ShotType type, const ShotExecutionTuning& tuning) {
        switch (type) {
        case ShotType::ControlledFinish:
            return tuning.controlledFinishDifficulty;
        case ShotType::PlacedShot:
            return tuning.placedShotDifficulty;
        case ShotType::PowerShot:
            return tuning.powerShotDifficulty;
        case ShotType::LongShot:
            return tuning.longShotDifficulty;
        case ShotType::TightAngleShot:
            return tuning.tightAngleShotDifficulty;
        case ShotType::DesperationShot:
            return tuning.desperationShotDifficulty;
        }
        return tuning.placedShotDifficulty;
    }

    double signedUnit(std::uint64_t seed) {
        return (matchEngineDeterministicUnitInterval(seed) * 2.0) - 1.0;
    }

    ShotTargetLane shiftedLane(ShotTargetLane lane, int shift) {
        int index = lane == ShotTargetLane::NearPost ? 0 : (lane == ShotTargetLane::Center ? 1 : 2);
        index = std::clamp(index + shift, 0, 2);
        return index == 0 ? ShotTargetLane::NearPost : (index == 1 ? ShotTargetLane::Center : ShotTargetLane::FarPost);
    }

    ShotTargetHeight shiftedHeight(ShotTargetHeight height, int shift) {
        int index = height == ShotTargetHeight::Low ? 0 : (height == ShotTargetHeight::Mid ? 1 : 2);
        index = std::clamp(index + shift, 0, 2);
        return index == 0 ? ShotTargetHeight::Low : (index == 1 ? ShotTargetHeight::Mid : ShotTargetHeight::High);
    }

    PitchPoint pitchPointForFrameTarget(
        ShotTargetPoint target,
        AttackingDirection direction) {
        const double goalX = direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters
            : 0.0;
        return PitchPoint{
            goalX,
            PitchGeometry::WidthMeters / 2.0 + target.lateral
        };
    }
}

ShotExecutionResult ShotExecutionModel::execute(const ShotExecutionRequest& request) const {
    const ShootingModelTuning modelTuning;
    const ShotExecutionTuning& tuning = modelTuning.execution;
    const ShotContext& context = request.context;
    const double shooting = attributeOrDefault(context.shooterAttributes.technical.shooting);
    const double technique = attributeOrDefault(context.shooterAttributes.technical.technique);
    const double composure = attributeOrDefault(context.shooterAttributes.mental.composure);
    const double decisions = attributeOrDefault(context.shooterAttributes.mental.decisions);
    const double agility = attributeOrDefault(context.shooterAttributes.physical.agility);
    const double skill =
        (shooting * tuning.shootingSkillWeight)
        + (technique * tuning.techniqueSkillWeight)
        + (composure * tuning.composureSkillWeight)
        + (decisions * tuning.decisionsSkillWeight)
        + (agility * tuning.agilitySkillWeight);

    const double difficulty =
        typeDifficulty(request.shotType, tuning)
        + (context.pressure * tuning.pressureDifficultyWeight)
        + (context.distanceMeters * tuning.distanceDifficultyWeight)
        + ((1.0 - context.centrality) * tuning.angleDifficultyWeight)
        + (context.lanePressure * tuning.lanePressureDifficultyWeight);
    const double qualityNoise = signedUnit(context.seed ^ 0x3ee171ULL) * tuning.executionNoiseRange;
    const double executionQuality = std::clamp(
        tuning.executionQualityBase + ((skill - difficulty) * tuning.skillDifficultyBlend) + qualityNoise,
        tuning.minimumExecutionQuality,
        tuning.maximumExecutionQuality);

    const double qualityPenalty = (100.0 - executionQuality) / 100.0;
    const double baseXG = ShotQualityModel::calculateOpenPlayXG(
        context.shotOrigin,
        context.attackingDirection,
        context.pressure);
    const double highXGControl = std::clamp(
        baseXG / std::max(tuning.highXGDeviationReference, 0.001),
        0.0,
        1.0);
    const double deviationReduction = 1.0 - highXGControl * tuning.highXGDeviationReduction;
    const double deviationBudget =
        (tuning.baseDeviationMeters
            + (qualityPenalty * tuning.qualityDeviationScale)
            + (context.pressure / 100.0 * tuning.pressureDeviationScale)
            + (context.distanceMeters * tuning.distanceDeviationScale)
            + ((1.0 - context.centrality) * tuning.angleDeviationScale)
            + (typeDifficulty(request.shotType, tuning) / tuning.difficultyScale * tuning.typeDeviationScale))
        * deviationReduction;
    const double directionError = signedUnit(context.seed ^ 0x6711b9ULL) * deviationBudget;
    const double heightError =
        signedUnit(context.seed ^ 0x54bd21ULL)
            * (tuning.heightErrorBase + qualityPenalty * tuning.heightErrorQualityScale)
            * deviationReduction;
    const double frameLateralDeviation =
        signedUnit(context.seed ^ 0x7a11e5ULL)
        * (tuning.frameBaseLateralDeviationMeters
            + qualityPenalty * tuning.frameQualityLateralDeviationScale
            + context.pressure / 100.0 * tuning.framePressureLateralDeviationScale
            + context.distanceMeters * tuning.frameDistanceLateralDeviationScale
            + (1.0 - context.centrality) * tuning.frameAngleLateralDeviationScale)
        * deviationReduction;
    const double frameHeightDeviation =
        signedUnit(context.seed ^ 0x827acdULL)
        * (tuning.frameBaseHeightDeviationMeters
            + qualityPenalty * tuning.frameQualityHeightDeviationScale
            + context.pressure / 100.0 * tuning.framePressureHeightDeviationScale
            + context.distanceMeters * tuning.frameDistanceHeightDeviationScale)
        * deviationReduction;

    ShotTargetZone actualZone = request.intendedTarget.intendedZone;
    if (std::abs(directionError) > tuning.laneShiftThreshold) {
        actualZone.lane = shiftedLane(actualZone.lane, directionError > 0.0 ? 1 : -1);
    }
    if (std::abs(heightError) > tuning.heightShiftThreshold) {
        actualZone.height = shiftedHeight(actualZone.height, heightError > 0.0 ? 1 : -1);
    }

    ShotTargetPoint actualFrameTarget{
        request.intendedTarget.intendedFrameTarget.lateral + frameLateralDeviation,
        request.intendedTarget.intendedFrameTarget.height + frameHeightDeviation
    };

    PitchPoint actualTarget = pitchPointForFrameTarget(actualFrameTarget, context.attackingDirection);
    actualTarget.y += directionError * 0.35;
    actualTarget.y = std::clamp(actualTarget.y, 0.0, PitchGeometry::WidthMeters);

    double powerBase = tuning.powerBase
        + (shooting - tuning.powerSkillBaseline) * tuning.shootingPowerWeight
        + (technique - tuning.powerSkillBaseline) * tuning.techniquePowerWeight;
    if (request.shotType == ShotType::PowerShot) {
        powerBase += tuning.powerShotPowerBonus;
    } else if (request.shotType == ShotType::LongShot) {
        powerBase += tuning.longShotPowerBonus;
    } else if (request.shotType == ShotType::ControlledFinish || request.shotType == ShotType::PlacedShot) {
        powerBase -= request.shotType == ShotType::ControlledFinish
            ? tuning.controlledFinishPowerPenalty
            : tuning.placedShotPowerPenalty;
    }
    const double shotPower = std::clamp(
        powerBase + signedUnit(context.seed ^ 0x9017adULL) * tuning.powerNoiseRange,
        tuning.minimumShotPower,
        tuning.maximumShotPower);

    return ShotExecutionResult{
        actualZone,
        actualTarget,
        actualFrameTarget,
        executionQuality,
        std::abs(frameLateralDeviation),
        std::abs(frameHeightDeviation),
        shotPower,
        std::clamp(
            request.intendedTarget.placementQuality
                + (executionQuality - tuning.placementExecutionBaseline) * tuning.placementExecutionBlend,
            0.0,
            100.0),
        executionQuality >= tuning.cleanStrikeThreshold
    };
}
