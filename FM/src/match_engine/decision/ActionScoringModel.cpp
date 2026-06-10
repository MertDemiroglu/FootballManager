#include"fm/match_engine/decision/ActionScoringModel.h"

#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>

namespace {
    double clampSelectionScore(double score) {
        return std::clamp(score, 0.0, 100.0);
    }

    double directionSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    PitchPoint advanceTarget(PitchPoint position, AttackingDirection direction, double meters) {
        return PitchGeometry::clampToPitch(PitchPoint{
            position.x + directionSign(direction) * meters,
            position.y
        });
    }

    bool isOwnDefensiveDanger(PitchPoint point, AttackingDirection direction) {
        if (direction == AttackingDirection::HomeToAway) {
            return point.x <= 22.0 || PitchGeometry::isInsideHomePenaltyArea(point);
        }

        return point.x >= PitchGeometry::LengthMeters - 22.0
            || PitchGeometry::isInsideAwayPenaltyArea(point);
    }

    bool isChancePass(PassOptionKind kind) {
        return kind == PassOptionKind::ThroughBall
            || kind == PassOptionKind::Cross
            || kind == PassOptionKind::Cutback;
    }

    double phaseFitFor(
        const PlayerDecisionContext& context,
        BallCarrierActionType type,
        const PhaseFitProfile& profile) {
        if (type == BallCarrierActionType::Shoot) {
            if (context.phase == DecisionMatchPhase::BoxEntry) {
                return profile.shotBoxEntry;
            }
            if (context.phase == DecisionMatchPhase::ChanceCreation) {
                return profile.shotChanceCreation;
            }
            if (context.phase == DecisionMatchPhase::FinalThird) {
                return profile.shotFinalThird;
            }
            if (context.phase == DecisionMatchPhase::BuildUp) {
                return profile.shotBuildUp;
            }
        }

        if ((type == BallCarrierActionType::Carry || type == BallCarrierActionType::Dribble)
            && context.possession.progressionAvailable) {
            return profile.carryProgressionAvailable;
        }

        return 0.0;
    }

    double weightedFinalScore(const ActionScoreBreakdown& breakdown) {
        const ActionScoringWeights weights = defaultActionScoringWeights();
        return clampSelectionScore(
            breakdown.retentionValue * weights.retentionWeight
            + breakdown.progressionValue * weights.progressionWeight
            + breakdown.chanceValue * weights.chanceWeight
            + breakdown.tacticalFit * weights.tacticalFitWeight
            + breakdown.roleFit * weights.roleFitWeight
            + breakdown.skillFit * weights.skillFitWeight
            + breakdown.phaseFit * weights.phaseFitWeight
            - breakdown.pressureCost * weights.pressureCostWeight
            - breakdown.turnoverRiskCost * weights.turnoverRiskWeight);
    }

    ActionCandidate candidateFromBreakdown(
        BallCarrierActionType type,
        PitchPoint target,
        PlayerId targetPlayerId,
        ActionScoreBreakdown breakdown) {
        breakdown.finalScore = weightedFinalScore(breakdown);

        ActionCandidate candidate;
        candidate.type = type;
        candidate.intendedTarget = PitchGeometry::clampToPitch(target);
        candidate.targetPlayerId = targetPlayerId;
        candidate.tacticalScore = breakdown.tacticalFit + breakdown.roleFit;
        candidate.contextScore =
            breakdown.retentionValue + breakdown.progressionValue + breakdown.chanceValue + breakdown.phaseFit;
        candidate.skillConfidenceScore = breakdown.skillFit;
        candidate.pressurePenalty = breakdown.pressureCost + breakdown.turnoverRiskCost;
        candidate.finalScore = breakdown.finalScore;
        return candidate;
    }
}

ActionCandidate ActionScoringModel::buildPassCandidate(
    const PassOption& option,
    const ActionScoringContext& context) const {
    const RoleDecisionProfile role = roleDecisionProfile(context.player.role);
    const TacticalDecisionProfile tactics = tacticalDecisionProfile(context.player.tacticalSetup);
    const PassActionScoringProfile scoring = defaultPassActionScoringProfile();
    const PhaseFitProfile phase = defaultPhaseFitProfile();

    ActionScoreBreakdown breakdown;
    breakdown.retentionValue =
        option.safetyScore * scoring.safetyToRetention * role.retentionPreference * tactics.retentionIntent;
    breakdown.progressionValue =
        option.progressionScore * scoring.progressionToProgression * role.progressionPreference * tactics.progressionIntent;
    breakdown.chanceValue = isChancePass(option.kind)
        ? option.progressionScore * scoring.chancePassProgressionToChance * role.chancePreference * tactics.chanceIntent
        : 0.0;
    breakdown.tacticalFit = option.kind == PassOptionKind::SwitchPlay
        ? scoring.switchPlayTacticalFit * tactics.widthIntent
        : scoring.directPassTacticalFit * tactics.directnessIntent;
    breakdown.roleFit = isChancePass(option.kind)
        ? scoring.chancePassRoleFit * role.directPreference
        : scoring.retentionPassRoleFit * role.retentionPreference;
    breakdown.skillFit = std::max(
        0.0,
        scoring.executionDifficultySkillBase - option.executionDifficulty * scoring.executionDifficultyToSkillCost);
    breakdown.pressureCost =
        option.laneRisk * scoring.laneRiskToPressureCost
        + option.receiverPressure * scoring.receiverPressureToPressureCost;
    breakdown.turnoverRiskCost = option.executionDifficulty * scoring.executionDifficultyToTurnoverRisk
        / std::max(scoring.riskToleranceMinimum, role.passRiskTolerance);
    breakdown.phaseFit = phaseFitFor(context.player, option.actionType, phase);

    return candidateFromBreakdown(
        option.actionType,
        option.targetPoint,
        option.targetPlayerId,
        breakdown);
}

ActionCandidate ActionScoringModel::buildCarryCandidate(
    const CarryOption& option,
    const ActionScoringContext& context) const {
    const RoleDecisionProfile role = roleDecisionProfile(context.player.role);
    const TacticalDecisionProfile tactics = tacticalDecisionProfile(context.player.tacticalSetup);
    const CarryActionScoringProfile scoring = defaultCarryActionScoringProfile();
    const PhaseFitProfile phase = defaultPhaseFitProfile();

    ActionScoreBreakdown breakdown;
    breakdown.retentionValue = option.spaceScore * scoring.spaceToRetention * role.retentionPreference;
    breakdown.progressionValue =
        option.progressionScore * scoring.progressionToProgression * role.progressionPreference * tactics.progressionIntent;
    breakdown.tacticalFit = option.kind == CarryOptionKind::SafeCarry
        ? scoring.safeCarryTacticalFit * tactics.retentionIntent
        : scoring.riskCarryTacticalFit * tactics.riskTolerance;
    breakdown.roleFit = option.kind == CarryOptionKind::Dribble
        ? scoring.dribbleRoleFit * role.carryRiskTolerance
        : scoring.progressionRoleFit * role.progressionPreference;
    breakdown.skillFit = std::max(
        0.0,
        scoring.controlDifficultySkillBase - option.controlDifficulty * scoring.controlDifficultyToSkillCost);
    breakdown.pressureCost = option.pressureRisk * scoring.pressureRiskToPressureCost;
    breakdown.turnoverRiskCost =
        (option.controlDifficulty * scoring.controlDifficultyToTurnoverRisk
            + option.zoneLimitRisk * scoring.zoneLimitToTurnoverRisk)
        / std::max(scoring.riskToleranceMinimum, role.carryRiskTolerance);
    breakdown.phaseFit = phaseFitFor(context.player, option.actionType, phase);

    return candidateFromBreakdown(option.actionType, option.targetPoint, 0, breakdown);
}

ActionCandidate ActionScoringModel::buildShotCandidate(
    const ShotOption& option,
    const ActionScoringContext& context) const {
    const RoleDecisionProfile role = roleDecisionProfile(context.player.role);
    const TacticalDecisionProfile tactics = tacticalDecisionProfile(context.player.tacticalSetup);
    const ShotActionScoringProfile scoring = defaultShotActionScoringProfile();
    const PhaseFitProfile phase = defaultPhaseFitProfile();

    ActionScoreBreakdown breakdown;
    breakdown.chanceValue =
        option.estimatedXG * scoring.xgToChanceValue
        + option.angleScore * scoring.angleToChanceValue
        + option.distanceScore * scoring.distanceToChanceValue;
    breakdown.tacticalFit = scoring.tacticalChanceFit * tactics.chanceIntent;
    breakdown.roleFit = scoring.roleChanceFit * role.chancePreference;
    breakdown.skillFit = option.shooterConfidence * scoring.shooterConfidenceToSkillFit;
    breakdown.pressureCost = option.pressurePenalty * scoring.pressurePenaltyToPressureCost
        / std::max(scoring.riskToleranceMinimum, role.shotRiskTolerance);
    if (option.estimatedXG < scoring.nonClearChanceXG) {
        if (context.player.possession.safeCirculationAvailable) {
            breakdown.turnoverRiskCost += scoring.safeCirculationShotCost;
        }
        if (context.player.possession.possessionActionCount <= 2) {
            breakdown.turnoverRiskCost += scoring.earlyActionShotCost;
        }
    }
    breakdown.phaseFit = phaseFitFor(context.player, option.actionType, phase);

    return candidateFromBreakdown(option.actionType, option.targetPoint, 0, breakdown);
}

ActionCandidate ActionScoringModel::buildHoldCandidate(const ActionScoringContext& context) const {
    const RoleDecisionProfile role = roleDecisionProfile(context.player.role);
    const TacticalDecisionProfile tactics = tacticalDecisionProfile(context.player.tacticalSetup);
    const HoldActionScoringProfile scoring = defaultHoldActionScoringProfile();

    ActionScoreBreakdown breakdown;
    breakdown.retentionValue = scoring.retentionBase * role.retentionPreference * tactics.retentionIntent;
    breakdown.tacticalFit = scoring.tacticalRetentionFit * tactics.retentionIntent;
    breakdown.roleFit = scoring.roleRetentionFit * role.retentionPreference;
    breakdown.skillFit = scoring.skillBase;
    breakdown.pressureCost = context.player.localPressure * scoring.localPressureCost;
    return candidateFromBreakdown(BallCarrierActionType::Hold, context.player.ballPosition, 0, breakdown);
}

ActionCandidate ActionScoringModel::buildClearCandidate(const ActionScoringContext& context) const {
    const ClearActionScoringProfile scoring = defaultClearActionScoringProfile();

    ActionScoreBreakdown breakdown;
    if (isOwnDefensiveDanger(context.player.ballPosition, context.attackingDirection)) {
        breakdown.retentionValue = scoring.retentionBase;
        breakdown.progressionValue = scoring.progressionBase;
        breakdown.tacticalFit =
            context.player.tacticalSetup.mentality == TeamMentality::Defensive
                ? scoring.defensiveMentalityTacticalFit
                : scoring.normalTacticalFit;
        breakdown.roleFit = scoring.roleFit;
        breakdown.skillFit = scoring.skillBase;
        breakdown.pressureCost = context.player.localPressure * scoring.localPressureCost;
    }

    return candidateFromBreakdown(
        BallCarrierActionType::Clear,
        advanceTarget(context.player.ballPosition, context.attackingDirection, scoring.targetAdvanceMeters),
        0,
        breakdown);
}
