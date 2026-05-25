#include"fm/match_engine/decision/BallCarrierDecisionModel.h"

#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

#include<algorithm>
#include<cmath>

namespace {
    struct CategorySuitability {
        double pass = 1.0;
        double carry = 1.0;
        double shoot = 1.0;
    };

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

    bool isAttackingRole(FormationSlotRole role) {
        return role == FormationSlotRole::AttackingMidfielder
            || role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger
            || role == FormationSlotRole::Striker;
    }

    double roleShotBias(FormationSlotRole role) {
        if (role == FormationSlotRole::Goalkeeper) {
            return 0.20;
        }
        if (role == FormationSlotRole::CenterBack) {
            return 0.55;
        }
        if (isAttackingRole(role)) {
            return 1.12;
        }
        return 1.0;
    }

    double tacticShotBias(const TacticalSetup& tactics) {
        if (tactics.mentality == TeamMentality::Attacking) {
            return 1.08;
        }
        if (tactics.mentality == TeamMentality::Defensive) {
            return 0.90;
        }
        return 1.0;
    }

    CategorySuitability evaluateCategorySuitability(const PlayerDecisionContext& context) {
        CategorySuitability suitability;
        const double pressure = std::clamp(context.localPressure / 100.0, 0.0, 1.0);
        const double progression = std::clamp(context.possession.ballProgression, 0.0, 1.0);

        suitability.pass = 1.0 + pressure * 0.18;
        suitability.carry = 1.0 - pressure * 0.22;
        suitability.shoot = (0.72 + progression * 0.48)
            * roleShotBias(context.role)
            * tacticShotBias(context.tacticalSetup);

        if (context.phase == DecisionMatchPhase::BoxEntry
            || context.phase == DecisionMatchPhase::ChanceCreation) {
            suitability.shoot *= 1.18;
            suitability.carry *= 0.96;
        }
        if (context.phase == DecisionMatchPhase::BuildUp) {
            suitability.shoot *= 0.62;
        }
        if (context.possession.progressionAvailable) {
            suitability.carry *= 1.05;
        }

        suitability.pass = std::clamp(suitability.pass, 0.65, 1.25);
        suitability.carry = std::clamp(suitability.carry, 0.55, 1.20);
        suitability.shoot = std::clamp(suitability.shoot, 0.20, 1.45);
        return suitability;
    }

    ActionCandidate buildCandidate(
        BallCarrierActionType type,
        PitchPoint target,
        double tacticalScore,
        double contextScore,
        double skillConfidenceScore,
        double pressurePenalty) {
        ActionCandidate candidate;
        candidate.type = type;
        candidate.intendedTarget = PitchGeometry::clampToPitch(target);
        candidate.tacticalScore = tacticalScore;
        candidate.contextScore = contextScore;
        candidate.skillConfidenceScore = skillConfidenceScore;
        candidate.pressurePenalty = pressurePenalty;
        candidate.finalScore = clampSelectionScore(
            tacticalScore + contextScore + skillConfidenceScore - pressurePenalty);
        return candidate;
    }

    ActionCandidate fromPassOption(const PassOption& option, double categorySuitability) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.targetPlayerId = option.targetPlayerId;
        candidate.tacticalScore = option.score * 0.42;
        candidate.contextScore = option.safetyScore * 0.22 + option.progressionScore * 0.18;
        candidate.skillConfidenceScore = std::max(0.0, 22.0 - option.executionDifficulty * 0.20);
        candidate.pressurePenalty = option.laneRisk * 0.12 + option.receiverPressure * 0.08;
        candidate.finalScore = clampSelectionScore(option.score * categorySuitability);
        return candidate;
    }

    ActionCandidate fromCarryOption(const CarryOption& option, double categorySuitability) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.tacticalScore = option.score * 0.40;
        candidate.contextScore = option.spaceScore * 0.18 + option.progressionScore * 0.20;
        candidate.skillConfidenceScore = std::max(0.0, 20.0 - option.controlDifficulty * 0.16);
        candidate.pressurePenalty = option.pressureRisk * 0.14 + option.zoneLimitRisk * 0.16;
        candidate.finalScore = clampSelectionScore(option.score * categorySuitability);
        return candidate;
    }

    ActionCandidate fromShotOption(const ShotOption& option, double categorySuitability) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.tacticalScore = option.score * 0.46;
        candidate.contextScore =
            option.estimatedXG * 80.0 + option.angleScore * 0.08 + option.distanceScore * 0.08;
        candidate.skillConfidenceScore = option.shooterConfidence * 0.10;
        candidate.pressurePenalty = option.pressurePenalty * 0.16;
        candidate.finalScore = clampSelectionScore(option.score * categorySuitability);
        return candidate;
    }
}

std::vector<ActionCandidate> BallCarrierDecisionModel::evaluate(
    const BallCarrierDecisionModelContext& context) const {
    std::vector<ActionCandidate> candidates;
    if (context.carrierState == nullptr) {
        return candidates;
    }

    const PlayerDecisionContext& player = context.playerContext;
    const CategorySuitability categories = evaluateCategorySuitability(player);
    candidates.push_back(buildCandidate(
        BallCarrierActionType::Hold,
        player.ballPosition,
        18.0 + (player.tacticalSetup.mentality == TeamMentality::Defensive ? 5.0 : 0.0),
        player.possession.teamInPossession == player.teamId ? 12.0 : 4.0,
        8.0,
        player.localPressure * 0.45));

    const std::vector<PassOption> passOptions = PassOptionEvaluator{}.evaluate(
        PassOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            player.role,
            player.tacticalSetup,
            player.ballPosition,
            context.attackingDirection,
            player.localPressure
        });
    for (const PassOption& option : passOptions) {
        candidates.push_back(fromPassOption(option, categories.pass));
    }

    const std::vector<CarryOption> carryOptions = CarryOptionEvaluator{}.evaluate(
        CarryOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            player.role,
            player.tacticalSetup,
            player.ballPosition,
            context.attackingDirection,
            player.localPressure
        });
    for (const CarryOption& option : carryOptions) {
        candidates.push_back(fromCarryOption(option, categories.carry));
    }

    const std::vector<ShotOption> shotOptions = ShotDecisionEvaluator{}.evaluate(
        ShotOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            player.role,
            player.tacticalSetup,
            player.ballPosition,
            context.attackingDirection,
            player.localPressure
        });
    for (const ShotOption& option : shotOptions) {
        candidates.push_back(fromShotOption(option, categories.shoot));
    }

    if (isOwnDefensiveDanger(player.ballPosition, context.attackingDirection)) {
        candidates.push_back(buildCandidate(
            BallCarrierActionType::Clear,
            advanceTarget(player.ballPosition, context.attackingDirection, 35.0),
            32.0 + (player.tacticalSetup.mentality == TeamMentality::Defensive ? 10.0 : 0.0),
            24.0,
            8.0,
            player.localPressure * 0.2));
    }

    return candidates;
}
