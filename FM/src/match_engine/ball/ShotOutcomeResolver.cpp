#include"fm/match_engine/ball/ShotOutcomeResolver.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>

namespace {
    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double attributeOrDefault(int value) {
        const ShootingModelTuning tuning;
        return value > 0 ? clampDouble(static_cast<double>(value), 0.0, 100.0) : tuning.defaultAttribute;
    }

    double goalkeeperSaveSkill(const PlayerAttributes& attributes, double fallbackStrength) {
        return clampDouble(
            attributeOrDefault(attributes.goalkeeper.shotStopping) * 0.36
                + attributeOrDefault(attributes.goalkeeper.oneOnOnes) * 0.22
                + attributeOrDefault(attributes.goalkeeper.handling) * 0.18
                + attributeOrDefault(attributes.goalkeeper.aerialAbility) * 0.08
                + attributeOrDefault(attributes.mental.positioning) * 0.08
                + attributeOrDefault(attributes.mental.concentration) * 0.08
                + fallbackStrength * 0.10,
            0.0,
            100.0);
    }
}

bool ShotAccuracyResolver::isOnTarget(const ShotOutcomeContext& context) const {
    const ShootingModelTuning tuning;
    const double executionMissFactor = (100.0 - context.execution.executionQuality) / 100.0;
    const double pressureFactor = clampDouble(context.shotContext.pressure / 100.0, 0.0, 1.0);
    const double angleFactor = clampDouble(1.0 - (context.shotContext.centrality * 4.0), 0.0, 1.0);
    const double difficultyFactor = clampDouble(context.quality.onTargetDifficulty / 100.0, 0.0, 1.0);
    const double deviationFactor = clampDouble(
        (context.execution.targetDeviationMeters * 0.16) + (context.execution.heightError * 0.20),
        0.0,
        0.45);

    double typePenalty = 0.0;
    if (context.shotType == ShotType::LongShot) {
        typePenalty = 0.08;
    } else if (context.shotType == ShotType::TightAngleShot) {
        typePenalty = 0.10;
    } else if (context.shotType == ShotType::DesperationShot) {
        typePenalty = 0.14;
    }

    const double offTargetProbability = clampDouble(
        tuning.offTargetBaseProbability
            + executionMissFactor * 0.30
            + pressureFactor * 0.12
            + angleFactor * 0.14
            + difficultyFactor * 0.12
            + deviationFactor
            + typePenalty,
        0.02,
        tuning.offTargetMaximumProbability);

    return matchEngineDeterministicUnitInterval(context.seed ^ 0xacc0ULL) >= offTargetProbability;
}

ShotOutcomeResult GoalkeeperSaveResolver::resolveOnTarget(const ShotOutcomeContext& context) const {
    const ShootingModelTuning tuning;
    const double keeperSkill = goalkeeperSaveSkill(
        context.shotContext.goalkeeperAttributes,
        context.shotContext.goalkeeperStrength);
    const double placement = clampDouble(context.execution.placementQuality / 100.0, 0.0, 1.0);
    const double power = clampDouble((context.execution.shotPower - 18.0) / 18.0, 0.0, 1.0);
    const double chanceQuality = clampDouble(context.quality.adjustedXG / 0.42, 0.0, 1.0);
    const double saveDifficulty = clampDouble(context.quality.saveDifficulty / 100.0, 0.0, 1.0);

    const double saveProbability = clampDouble(
        0.48
            + (keeperSkill - 55.0) * 0.006
            - chanceQuality * 0.30
            - placement * 0.13
            - power * 0.08
            - saveDifficulty * 0.10,
        tuning.saveMinimumProbability,
        tuning.saveMaximumProbability);
    const double saveRoll = matchEngineDeterministicUnitInterval(context.seed ^ 0x5a9eULL);
    if (saveRoll >= saveProbability) {
        return ShotOutcomeResult{ ShotOutcomeKind::Goal, true, true, false };
    }

    const double handling = attributeOrDefault(context.shotContext.goalkeeperAttributes.goalkeeper.handling);
    const double heldProbability = clampDouble(
        tuning.heldReboundBase
            + ((handling - 50.0) / 100.0) * tuning.handlingHeldScale
            + ((keeperSkill - 55.0) / 100.0) * 0.18
            - power * tuning.reboundPowerScale
            - context.quality.reboundRisk * 0.28,
        0.12,
        0.88);
    const bool held = matchEngineDeterministicUnitInterval(context.seed ^ 0xadd5ULL) < heldProbability;
    return held
        ? ShotOutcomeResult{ ShotOutcomeKind::SavedHeld, true, false, false }
        : ShotOutcomeResult{ ShotOutcomeKind::SavedRebound, true, false, true };
}

ShotOutcomeResult ShotOutcomeResolver::resolve(const ShotOutcomeContext& context) const {
    if (!ShotAccuracyResolver{}.isOnTarget(context)) {
        return ShotOutcomeResult{ ShotOutcomeKind::OffTarget, false, false, false };
    }
    return GoalkeeperSaveResolver{}.resolveOnTarget(context);
}
