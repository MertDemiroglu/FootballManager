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

    double phaseFitFor(const PlayerDecisionContext& context, BallCarrierActionType type) {
        if (type == BallCarrierActionType::Shoot) {
            if (context.phase == DecisionMatchPhase::BoxEntry
                || context.phase == DecisionMatchPhase::ChanceCreation) {
                return 8.0;
            }
            if (context.phase == DecisionMatchPhase::FinalThird) {
                return 4.0;
            }
            if (context.phase == DecisionMatchPhase::BuildUp) {
                return -8.0;
            }
        }

        if ((type == BallCarrierActionType::Carry || type == BallCarrierActionType::Dribble)
            && context.possession.progressionAvailable) {
            return 4.0;
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

    ActionScoreBreakdown breakdown;
    breakdown.retentionValue = option.safetyScore * 0.24 * role.retentionPreference * tactics.retentionIntent;
    breakdown.progressionValue =
        option.progressionScore * 0.24 * role.progressionPreference * tactics.progressionIntent;
    breakdown.chanceValue = isChancePass(option.kind)
        ? option.progressionScore * 0.12 * role.chancePreference * tactics.chanceIntent
        : 0.0;
    breakdown.tacticalFit = option.kind == PassOptionKind::SwitchPlay
        ? 5.0 * tactics.widthIntent
        : 3.0 * tactics.directnessIntent;
    breakdown.roleFit = isChancePass(option.kind)
        ? 4.0 * role.directPreference
        : 4.0 * role.retentionPreference;
    breakdown.skillFit = std::max(0.0, 22.0 - option.executionDifficulty * 0.20);
    breakdown.pressureCost = option.laneRisk * 0.12 + option.receiverPressure * 0.08;
    breakdown.turnoverRiskCost = option.executionDifficulty * 0.08 / std::max(0.45, role.passRiskTolerance);
    breakdown.phaseFit = phaseFitFor(context.player, option.actionType);

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

    ActionScoreBreakdown breakdown;
    breakdown.retentionValue = option.spaceScore * 0.20 * role.retentionPreference;
    breakdown.progressionValue =
        option.progressionScore * 0.30 * role.progressionPreference * tactics.progressionIntent;
    breakdown.tacticalFit = option.kind == CarryOptionKind::SafeCarry
        ? 4.0 * tactics.retentionIntent
        : 5.0 * tactics.riskTolerance;
    breakdown.roleFit = option.kind == CarryOptionKind::Dribble
        ? 4.0 * role.carryRiskTolerance
        : 4.0 * role.progressionPreference;
    breakdown.skillFit = std::max(0.0, 20.0 - option.controlDifficulty * 0.16);
    breakdown.pressureCost = option.pressureRisk * 0.14;
    breakdown.turnoverRiskCost =
        (option.controlDifficulty * 0.06 + option.zoneLimitRisk * 0.16)
        / std::max(0.45, role.carryRiskTolerance);
    breakdown.phaseFit = phaseFitFor(context.player, option.actionType);

    return candidateFromBreakdown(option.actionType, option.targetPoint, 0, breakdown);
}

ActionCandidate ActionScoringModel::buildShotCandidate(
    const ShotOption& option,
    const ActionScoringContext& context) const {
    const RoleDecisionProfile role = roleDecisionProfile(context.player.role);
    const TacticalDecisionProfile tactics = tacticalDecisionProfile(context.player.tacticalSetup);

    ActionScoreBreakdown breakdown;
    breakdown.chanceValue =
        option.estimatedXG * 80.0
        + option.angleScore * 0.08
        + option.distanceScore * 0.08;
    breakdown.tacticalFit = 5.0 * tactics.chanceIntent;
    breakdown.roleFit = 8.0 * role.chancePreference;
    breakdown.skillFit = option.shooterConfidence * 0.10;
    breakdown.pressureCost = option.pressurePenalty * 0.16 / std::max(0.45, role.shotRiskTolerance);
    breakdown.phaseFit = phaseFitFor(context.player, option.actionType);

    return candidateFromBreakdown(option.actionType, option.targetPoint, 0, breakdown);
}

ActionCandidate ActionScoringModel::buildHoldCandidate(const ActionScoringContext& context) const {
    const RoleDecisionProfile role = roleDecisionProfile(context.player.role);
    const TacticalDecisionProfile tactics = tacticalDecisionProfile(context.player.tacticalSetup);

    ActionScoreBreakdown breakdown;
    breakdown.retentionValue = 18.0 * role.retentionPreference * tactics.retentionIntent;
    breakdown.tacticalFit = 5.0 * tactics.retentionIntent;
    breakdown.roleFit = 4.0 * role.retentionPreference;
    breakdown.skillFit = 8.0;
    breakdown.pressureCost = context.player.localPressure * 0.45;
    return candidateFromBreakdown(BallCarrierActionType::Hold, context.player.ballPosition, 0, breakdown);
}

ActionCandidate ActionScoringModel::buildClearCandidate(const ActionScoringContext& context) const {
    ActionScoreBreakdown breakdown;
    if (isOwnDefensiveDanger(context.player.ballPosition, context.attackingDirection)) {
        breakdown.retentionValue = 12.0;
        breakdown.progressionValue = 22.0;
        breakdown.tacticalFit =
            context.player.tacticalSetup.mentality == TeamMentality::Defensive ? 10.0 : 4.0;
        breakdown.roleFit = 8.0;
        breakdown.skillFit = 8.0;
        breakdown.pressureCost = context.player.localPressure * 0.20;
    }

    return candidateFromBreakdown(
        BallCarrierActionType::Clear,
        advanceTarget(context.player.ballPosition, context.attackingDirection, 35.0),
        0,
        breakdown);
}
