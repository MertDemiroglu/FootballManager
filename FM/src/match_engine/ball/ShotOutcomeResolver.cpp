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

    double goalkeeperSaveSkill(
        const PlayerAttributes& attributes,
        double fallbackStrength,
        const ShotOutcomeTuning& tuning) {
        return clampDouble(
            attributeOrDefault(attributes.goalkeeper.shotStopping) * tuning.goalkeeperShotStoppingWeight
                + attributeOrDefault(attributes.goalkeeper.oneOnOnes) * tuning.goalkeeperOneOnOnesWeight
                + attributeOrDefault(attributes.goalkeeper.handling) * tuning.goalkeeperHandlingWeight
                + attributeOrDefault(attributes.goalkeeper.aerialAbility) * tuning.goalkeeperAerialAbilityWeight
                + attributeOrDefault(attributes.mental.positioning) * tuning.goalkeeperPositioningWeight
                + attributeOrDefault(attributes.mental.concentration) * tuning.goalkeeperConcentrationWeight
                + fallbackStrength * tuning.goalkeeperFallbackStrengthWeight,
            0.0,
            100.0);
    }
}

bool ShotAccuracyResolver::isOnTarget(const ShotOutcomeContext& context) const {
    const ShootingModelTuning modelTuning;
    const ShotOutcomeTuning& tuning = modelTuning.outcome;
    const double executionMissFactor = (100.0 - context.execution.executionQuality) / 100.0;
    const double pressureFactor = clampDouble(context.shotContext.pressure / 100.0, 0.0, 1.0);
    const double angleFactor = clampDouble(1.0 - (context.shotContext.centrality * 4.0), 0.0, 1.0);
    const double difficultyFactor = clampDouble(context.quality.onTargetDifficulty / 100.0, 0.0, 1.0);
    const double deviationFactor = clampDouble(
        (context.execution.targetDeviationMeters * tuning.deviationMissWeight)
            + (context.execution.heightError * tuning.heightErrorMissWeight),
        0.0,
        tuning.deviationFactorMaximum);

    double typePenalty = 0.0;
    if (context.shotType == ShotType::LongShot) {
        typePenalty = tuning.longShotMissPenalty;
    } else if (context.shotType == ShotType::TightAngleShot) {
        typePenalty = tuning.tightAngleShotMissPenalty;
    } else if (context.shotType == ShotType::DesperationShot) {
        typePenalty = tuning.desperationShotMissPenalty;
    }

    const double offTargetProbability = clampDouble(
        tuning.offTargetBaseProbability
            + executionMissFactor * tuning.executionMissWeight
            + pressureFactor * tuning.pressureMissWeight
            + angleFactor * tuning.angleMissWeight
            + difficultyFactor * tuning.difficultyMissWeight
            + deviationFactor
            + typePenalty,
        tuning.offTargetMinimumProbability,
        tuning.offTargetMaximumProbability);

    return matchEngineDeterministicUnitInterval(context.seed ^ 0xacc0ULL) >= offTargetProbability;
}

ShotOutcomeResult GoalkeeperSaveResolver::resolveOnTarget(const ShotOutcomeContext& context) const {
    const ShootingModelTuning modelTuning;
    const ShotOutcomeTuning& tuning = modelTuning.outcome;
    const double keeperSkill = goalkeeperSaveSkill(
        context.shotContext.goalkeeperAttributes,
        context.shotContext.goalkeeperStrength,
        tuning);
    const double placement = clampDouble(context.execution.placementQuality / 100.0, 0.0, 1.0);
    const double power = clampDouble(
        (context.execution.shotPower - tuning.shotPowerBaseline) / tuning.shotPowerScale,
        0.0,
        1.0);
    const double chanceQuality = clampDouble(context.quality.adjustedXG / tuning.chanceQualityScale, 0.0, 1.0);
    const double saveDifficulty = clampDouble(context.quality.saveDifficulty / 100.0, 0.0, 1.0);

    const double saveProbability = clampDouble(
        tuning.saveProbabilityBase
            + (keeperSkill - tuning.saveSkillBaseline) * tuning.saveSkillWeight
            - chanceQuality * tuning.saveChanceQualityWeight
            - placement * tuning.savePlacementWeight
            - power * tuning.savePowerWeight
            - saveDifficulty * tuning.saveDifficultyWeight,
        tuning.saveMinimumProbability,
        tuning.saveMaximumProbability);
    const double saveRoll = matchEngineDeterministicUnitInterval(context.seed ^ 0x5a9eULL);
    if (saveRoll >= saveProbability) {
        return ShotOutcomeResult{ ShotOutcomeKind::Goal, true, true, false };
    }

    const double handling = attributeOrDefault(context.shotContext.goalkeeperAttributes.goalkeeper.handling);
    const double heldProbability = clampDouble(
        tuning.heldReboundBase
            + ((handling - tuning.handlingBaseline) / 100.0) * tuning.handlingHeldScale
            + ((keeperSkill - tuning.saveSkillBaseline) / 100.0) * tuning.keeperSkillHeldScale
            - power * tuning.reboundPowerScale
            - context.quality.reboundRisk * tuning.reboundRiskHeldPenalty,
        tuning.heldMinimumProbability,
        tuning.heldMaximumProbability);
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
