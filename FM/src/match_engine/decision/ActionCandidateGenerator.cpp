#include"fm/match_engine/decision/ActionCandidateGenerator.h"

#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

#include<algorithm>
#include<cmath>
#include<limits>
#include<vector>

namespace {
    double clampScore(double value) {
        return std::clamp(value, 0.0, 100.0);
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

    double nearestOpponentPressure(
        PitchPoint position,
        const std::vector<PlayerSimState>& opponents) {
        double nearest = std::numeric_limits<double>::max();
        for (const PlayerSimState& opponent : opponents) {
            nearest = std::min(nearest, PitchGeometry::distance(position, opponent.position));
        }

        if (nearest <= 4.0) {
            return 35.0;
        }
        if (nearest <= 8.0) {
            return 22.0;
        }
        if (nearest <= 14.0) {
            return 10.0;
        }

        return opponents.empty() ? 0.0 : 3.0;
    }

    FormationSlotRole assignedRoleFor(const MatchTeamSnapshot* teamSnapshot, PlayerId playerId) {
        if (teamSnapshot == nullptr) {
            return FormationSlotRole::Unknown;
        }

        for (const TeamSheetSlotAssignment& assignment : teamSnapshot->teamSheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }

        return FormationSlotRole::Unknown;
    }
    double mentalitySafeBonus(TeamMentality mentality) {
        return mentality == TeamMentality::Defensive ? 10.0 : 0.0;
    }

    ActionCandidate buildCandidate(
        BallCarrierActionType type,
        PitchPoint target,
        PlayerId targetPlayerId,
        double tacticalScore,
        double contextScore,
        double skillConfidenceScore,
        double pressurePenalty) {
        ActionCandidate candidate;
        candidate.type = type;
        candidate.intendedTarget = PitchGeometry::clampToPitch(target);
        candidate.targetPlayerId = targetPlayerId;
        candidate.tacticalScore = tacticalScore;
        candidate.contextScore = contextScore;
        candidate.mentalScore = 0.0;
        candidate.skillConfidenceScore = skillConfidenceScore;
        candidate.pressurePenalty = pressurePenalty;
        candidate.finalScore = clampScore(
            tacticalScore + contextScore + skillConfidenceScore - pressurePenalty);
        return candidate;
    }

    ActionCandidate buildCandidateFromPassOption(const PassOption& option) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.targetPlayerId = option.targetPlayerId;
        candidate.tacticalScore = option.score * 0.42;
        candidate.contextScore = option.safetyScore * 0.22 + option.progressionScore * 0.18;
        candidate.mentalScore = 0.0;
        candidate.skillConfidenceScore = std::max(0.0, 22.0 - option.executionDifficulty * 0.20);
        candidate.pressurePenalty = option.laneRisk * 0.12 + option.receiverPressure * 0.08;
        double selectionScore = option.score;
        if (option.kind == PassOptionKind::SafePass || option.kind == PassOptionKind::BackPass) {
            selectionScore *= 0.88;
        } else {
            selectionScore *= 1.08;
        }
        candidate.finalScore = clampScore(selectionScore);
        return candidate;
    }

    ActionCandidate buildCandidateFromCarryOption(const CarryOption& option) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.targetPlayerId = 0;
        candidate.tacticalScore = option.score * 0.40;
        candidate.contextScore = option.spaceScore * 0.18 + option.progressionScore * 0.20;
        candidate.mentalScore = 0.0;
        candidate.skillConfidenceScore = std::max(0.0, 20.0 - option.controlDifficulty * 0.16);
        candidate.pressurePenalty = option.pressureRisk * 0.14 + option.zoneLimitRisk * 0.16;
        candidate.finalScore = clampScore(option.score);
        return candidate;
    }

    ActionCandidate buildCandidateFromShotOption(const ShotOption& option) {
        ActionCandidate candidate;
        candidate.type = option.actionType;
        candidate.intendedTarget = PitchGeometry::clampToPitch(option.targetPoint);
        candidate.targetPlayerId = 0;
        candidate.tacticalScore = option.score * 0.46;
        candidate.contextScore =
            option.estimatedXG * 80.0 + option.angleScore * 0.08 + option.distanceScore * 0.08;
        candidate.mentalScore = 0.0;
        candidate.skillConfidenceScore = option.shooterConfidence * 0.10;
        candidate.pressurePenalty = option.pressurePenalty * 0.16;
        candidate.finalScore = clampScore(option.score);
        return candidate;
    }
}

std::vector<ActionCandidate> ActionCandidateGenerator::generate(
    const ActionCandidateGenerationRequest& request) const {
    std::vector<ActionCandidate> candidates;

    const TacticalSetup& tacticalSetup = request.teamShapeContext.tacticalSetup;
    const AttackingDirection direction = request.teamShapeContext.attackingDirection;
    const PitchPoint carrierPosition = request.ballCarrier.position;
    const double pressurePenalty = nearestOpponentPressure(carrierPosition, request.opponents);
    const TeamSimState* requestTeamState =
        findTeamState(request.simulationState, request.ballCarrier.teamId);
    const TeamSimState* requestOpponentState =
        request.simulationState.homeTeam.teamId == request.ballCarrier.teamId
            ? &request.simulationState.awayTeam
            : &request.simulationState.homeTeam;
    const bool controlledBall =
        request.ballState.controlState == BallControlState::Controlled
        && request.ballState.carrierPlayerId == request.ballCarrier.playerId;
    const FormationSlotRole carrierRole = assignedRoleFor(
        request.teamSnapshot,
        request.ballCarrier.playerId);

    candidates.push_back(buildCandidate(
        BallCarrierActionType::Hold,
        carrierPosition,
        0,
        24.0 + mentalitySafeBonus(tacticalSetup.mentality),
        controlledBall ? 12.0 : 4.0,
        8.0,
        pressurePenalty * 0.45));

    const std::vector<PassOption> passOptions = PassOptionEvaluator{}.evaluate(
        PassOptionEvaluationContext{
            request.teamSnapshot,
            request.opponentSnapshot,
            requestTeamState,
            requestOpponentState,
            &request.ballCarrier,
            carrierRole,
            tacticalSetup,
            carrierPosition,
            direction,
            pressurePenalty
        });
    for (const PassOption& option : passOptions) {
        candidates.push_back(buildCandidateFromPassOption(option));
    }

    const std::vector<CarryOption> carryOptions = CarryOptionEvaluator{}.evaluate(
        CarryOptionEvaluationContext{
            request.teamSnapshot,
            request.opponentSnapshot,
            requestTeamState,
            requestOpponentState,
            &request.ballCarrier,
            carrierRole,
            tacticalSetup,
            carrierPosition,
            direction,
            pressurePenalty
        });
    for (const CarryOption& option : carryOptions) {
        candidates.push_back(buildCandidateFromCarryOption(option));
    }

    const std::vector<ShotOption> shotOptions = ShotDecisionEvaluator{}.evaluate(
        ShotOptionEvaluationContext{
            request.teamSnapshot,
            request.opponentSnapshot,
            requestTeamState,
            requestOpponentState,
            &request.ballCarrier,
            carrierRole,
            tacticalSetup,
            carrierPosition,
            direction,
            pressurePenalty
        });
    for (const ShotOption& option : shotOptions) {
        candidates.push_back(buildCandidateFromShotOption(option));
    }

    if (isOwnDefensiveDanger(carrierPosition, direction)) {
        candidates.push_back(buildCandidate(
            BallCarrierActionType::Clear,
            advanceTarget(carrierPosition, direction, 35.0),
            0,
            32.0 + mentalitySafeBonus(tacticalSetup.mentality),
            24.0,
            8.0,
            pressurePenalty * 0.2));
    }

    if (request.simulationState.possession.isTransition) {
        for (ActionCandidate& candidate : candidates) {
            if (candidate.type == BallCarrierActionType::Carry
                || candidate.type == BallCarrierActionType::ShortPass) {
                candidate.contextScore += 4.0;
            }
            candidate.finalScore = clampScore(
                candidate.tacticalScore
                + candidate.contextScore
                + candidate.skillConfidenceScore
                - candidate.pressurePenalty);
        }
    }

    return candidates;
}
