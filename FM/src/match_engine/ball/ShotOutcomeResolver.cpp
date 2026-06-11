#include"fm/match_engine/ball/ShotOutcomeResolver.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<cmath>

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
    return GoalFrame{}.contains(context.execution.actualFrameTarget);
}

bool ShotAccuracyResolver::isWoodwork(const ShotOutcomeContext& context) const {
    const ShootingModelTuning modelTuning;
    return GoalFrame{}.touchesFrame(
        context.execution.actualFrameTarget,
        modelTuning.outcome.woodworkToleranceMeters);
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
    const double frameHalfWidth = GoalFrame{}.width / 2.0;
    const double keeperLateral =
        context.shotContext.goalkeeperPosition.y - PitchGeometry::WidthMeters / 2.0;
    const double lateralReachDemand = std::abs(context.execution.actualFrameTarget.lateral - keeperLateral)
        / std::max(tuning.keeperLateralReachMeters, 0.001);
    const double verticalReachDemand =
        context.execution.actualFrameTarget.height / std::max(tuning.keeperVerticalReachMeters, 0.001);
    const double reachDifficulty = clampDouble(
        (lateralReachDemand + verticalReachDemand) * 0.5,
        0.0,
        1.0);
    const double framePlacement = clampDouble(
        (std::abs(context.execution.actualFrameTarget.lateral) / std::max(frameHalfWidth, 0.001)) * 0.65
            + (context.execution.actualFrameTarget.height / std::max(GoalFrame{}.height, 0.001)) * 0.35,
        0.0,
        1.0);

    const double shotThreatPenalty = clampDouble(
        chanceQuality * tuning.saveChanceQualityWeight
            + placement * tuning.savePlacementWeight
            + power * tuning.savePowerWeight
            + saveDifficulty * tuning.saveDifficultyWeight
            + framePlacement * tuning.framePlacementWeight
            + reachDifficulty * tuning.keeperReachDifficultyWeight,
        0.0,
        tuning.maximumShotThreatPenalty);

    const double saveProbability = clampDouble(
        tuning.saveProbabilityBase
            + (keeperSkill - tuning.saveSkillBaseline) * tuning.saveSkillWeight
            - shotThreatPenalty,
        tuning.saveMinimumProbability,
        tuning.saveMaximumProbability);
    const double saveRoll = matchEngineDeterministicUnitInterval(context.seed ^ 0x5a9eULL);
    if (saveRoll >= saveProbability) {
        return ShotOutcomeResult{ ShotOutcomeKind::Goal, true, true, false, false };
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
        ? ShotOutcomeResult{ ShotOutcomeKind::SavedHeld, true, false, false, false }
        : ShotOutcomeResult{ ShotOutcomeKind::SavedRebound, true, false, true, false };
}

ShotOutcomeResult ShotOutcomeResolver::resolve(const ShotOutcomeContext& context) const {
    const ShotAccuracyResolver accuracy;
    if (!accuracy.isOnTarget(context)) {
        if (accuracy.isWoodwork(context)) {
            return ShotOutcomeResult{ ShotOutcomeKind::Woodwork, false, false, false, true };
        }

        return ShotOutcomeResult{ ShotOutcomeKind::OffTarget, false, false, false, false };
    }
    return GoalkeeperSaveResolver{}.resolveOnTarget(context);
}
