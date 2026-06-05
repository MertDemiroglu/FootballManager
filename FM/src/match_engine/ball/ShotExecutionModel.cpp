#include"fm/match_engine/ball/ShotExecutionModel.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/ball/ShotTargetSelector.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<cmath>

namespace {
    double attributeOrDefault(int value) {
        const ShootingModelTuning tuning;
        return value > 0 ? std::clamp(static_cast<double>(value), 0.0, 100.0) : tuning.defaultAttribute;
    }

    double typeDifficulty(ShotType type) {
        switch (type) {
        case ShotType::ControlledFinish:
            return 10.0;
        case ShotType::PlacedShot:
            return 17.0;
        case ShotType::PowerShot:
            return 23.0;
        case ShotType::LongShot:
            return 34.0;
        case ShotType::TightAngleShot:
            return 38.0;
        case ShotType::DesperationShot:
            return 48.0;
        }
        return 20.0;
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
}

ShotExecutionResult ShotExecutionModel::execute(const ShotExecutionRequest& request) const {
    const ShootingModelTuning tuning;
    const ShotContext& context = request.context;
    const double shooting = attributeOrDefault(context.shooterAttributes.technical.shooting);
    const double technique = attributeOrDefault(context.shooterAttributes.technical.technique);
    const double composure = attributeOrDefault(context.shooterAttributes.mental.composure);
    const double decisions = attributeOrDefault(context.shooterAttributes.mental.decisions);
    const double agility = attributeOrDefault(context.shooterAttributes.physical.agility);
    const double skill =
        (shooting * 0.34)
        + (technique * 0.26)
        + (composure * 0.19)
        + (decisions * 0.13)
        + (agility * 0.08);

    const double difficulty =
        typeDifficulty(request.shotType)
        + (context.pressure * 0.32)
        + (context.distanceMeters * 0.42)
        + ((1.0 - context.centrality) * 24.0)
        + (context.lanePressure * 0.12);
    const double qualityNoise = signedUnit(context.seed ^ 0x3ee171ULL) * 9.0;
    const double executionQuality = std::clamp(
        62.0 + ((skill - difficulty) * 0.72) + qualityNoise,
        1.0,
        99.0);

    const double qualityPenalty = (100.0 - executionQuality) / 100.0;
    const double deviationBudget =
        tuning.baseDeviationMeters
        + (qualityPenalty * 4.1)
        + (context.pressure / 100.0 * tuning.pressureDeviationScale)
        + (context.distanceMeters * tuning.distanceDeviationScale)
        + ((1.0 - context.centrality) * tuning.angleDeviationScale)
        + (typeDifficulty(request.shotType) / 100.0 * tuning.difficultTypeDeviationScale);
    const double directionError = signedUnit(context.seed ^ 0x6711b9ULL) * deviationBudget;
    const double heightError = signedUnit(context.seed ^ 0x54bd21ULL) * (0.35 + qualityPenalty * 1.8);

    ShotTargetZone actualZone = request.intendedTarget.intendedZone;
    if (std::abs(directionError) > 1.55) {
        actualZone.lane = shiftedLane(actualZone.lane, directionError > 0.0 ? 1 : -1);
    }
    if (std::abs(heightError) > 0.85) {
        actualZone.height = shiftedHeight(actualZone.height, heightError > 0.0 ? 1 : -1);
    }

    PitchPoint actualTarget =
        shotTargetPointFor(context.shotOrigin, context.attackingDirection, actualZone);
    actualTarget.y += directionError;
    actualTarget.y = std::clamp(actualTarget.y, 0.0, PitchGeometry::WidthMeters);

    double powerBase = 24.0 + (shooting - 50.0) * 0.045 + (technique - 50.0) * 0.025;
    if (request.shotType == ShotType::PowerShot || request.shotType == ShotType::LongShot) {
        powerBase += 4.0;
    } else if (request.shotType == ShotType::ControlledFinish || request.shotType == ShotType::PlacedShot) {
        powerBase -= 1.8;
    }
    const double shotPower = std::clamp(
        powerBase + signedUnit(context.seed ^ 0x9017adULL) * 2.5,
        tuning.minimumShotPower,
        tuning.maximumShotPower);

    return ShotExecutionResult{
        actualZone,
        actualTarget,
        executionQuality,
        std::abs(directionError),
        std::abs(heightError),
        shotPower,
        std::clamp(request.intendedTarget.placementQuality + (executionQuality - 55.0) * 0.35, 0.0, 100.0),
        executionQuality >= tuning.cleanStrikeThreshold
    };
}
