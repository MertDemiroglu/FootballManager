#include"fm/match_engine/decision/BallCarrierDecisionModel.h"

#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

#include<algorithm>

namespace {
    double clampSelectionScore(double score) {
        return std::clamp(score, 0.0, 100.0);
    }

    ActionCandidate fromPassOption(const PassOption& option) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.targetPlayerId = option.targetPlayerId;
        candidate.tacticalScore = option.score * 0.42;
        candidate.contextScore = option.safetyScore * 0.22 + option.progressionScore * 0.18;
        candidate.skillConfidenceScore = std::max(0.0, 22.0 - option.executionDifficulty * 0.20);
        candidate.pressurePenalty = option.laneRisk * 0.12 + option.receiverPressure * 0.08;
        candidate.finalScore = clampSelectionScore(option.score);
        return candidate;
    }

    ActionCandidate fromCarryOption(const CarryOption& option) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.tacticalScore = option.score * 0.40;
        candidate.contextScore = option.spaceScore * 0.18 + option.progressionScore * 0.20;
        candidate.skillConfidenceScore = std::max(0.0, 20.0 - option.controlDifficulty * 0.16);
        candidate.pressurePenalty = option.pressureRisk * 0.14 + option.zoneLimitRisk * 0.16;
        candidate.finalScore = clampSelectionScore(option.score);
        return candidate;
    }

    ActionCandidate fromShotOption(const ShotOption& option) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.tacticalScore = option.score * 0.46;
        candidate.contextScore =
            option.estimatedXG * 80.0 + option.angleScore * 0.08 + option.distanceScore * 0.08;
        candidate.skillConfidenceScore = option.shooterConfidence * 0.10;
        candidate.pressurePenalty = option.pressurePenalty * 0.16;
        candidate.finalScore = clampSelectionScore(option.score);
        return candidate;
    }
}

std::vector<ActionCandidate> BallCarrierDecisionModel::evaluate(
    const BallCarrierDecisionModelContext& context) const {
    std::vector<ActionCandidate> candidates;

    const std::vector<PassOption> passOptions = PassOptionEvaluator{}.evaluate(
        PassOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            context.carrierRole,
            context.tacticalSetup,
            context.ballPosition,
            context.attackingDirection,
            context.carrierPressure
        });
    for (const PassOption& option : passOptions) {
        candidates.push_back(fromPassOption(option));
    }

    const std::vector<CarryOption> carryOptions = CarryOptionEvaluator{}.evaluate(
        CarryOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            context.carrierRole,
            context.tacticalSetup,
            context.ballPosition,
            context.attackingDirection,
            context.carrierPressure
        });
    for (const CarryOption& option : carryOptions) {
        candidates.push_back(fromCarryOption(option));
    }

    const std::vector<ShotOption> shotOptions = ShotDecisionEvaluator{}.evaluate(
        ShotOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            context.carrierRole,
            context.tacticalSetup,
            context.ballPosition,
            context.attackingDirection,
            context.carrierPressure
        });
    for (const ShotOption& option : shotOptions) {
        candidates.push_back(fromShotOption(option));
    }

    return candidates;
}
