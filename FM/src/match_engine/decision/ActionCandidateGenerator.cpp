#include"fm/match_engine/decision/ActionCandidateGenerator.h"

#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

#include<algorithm>
#include<cmath>
#include<limits>
#include<vector>

namespace {
    // ActionCandidate::finalScore is the cross-action selection contract used by
    // ActionSelector, which raises scores with pow(finalScore, sharpness).
    // Keep pass, carry, and shot candidates on the same scale:
    // 10-20 weak/rare, 25-40 viable, 40-60 strong, 60+ very strong/uncommon.
    double clampScore(double value) {
        return std::clamp(value, 0.0, 100.0);
    }

    double clampCandidateScore(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double normalizeOptionScore(
        double optionScore,
        double base,
        double factor,
        double minimum,
        double maximum) {
        return clampCandidateScore(base + optionScore * factor, minimum, maximum);
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

    double progressionUrgency(const MatchSimulationState& state, const TacticalSetup& tactics) {
        double urgency = std::clamp(static_cast<double>(state.possession.actionDepth) / 7.0, 0.0, 1.0);
        if (tactics.mentality == TeamMentality::Defensive) {
            urgency *= 0.70;
        } else if (tactics.mentality == TeamMentality::Attacking) {
            urgency *= 1.20;
        }
        if (tactics.tempo == TeamTempo::Low) {
            urgency *= 0.75;
        } else if (tactics.tempo == TeamTempo::High) {
            urgency *= 1.15;
        }
        return std::clamp(urgency, 0.0, 1.25);
    }

    double contractClampFor(BallCarrierActionType type, double score) {
        const double maximum = type == BallCarrierActionType::Clear ? 82.0 : 75.0;
        return std::clamp(score, 0.0, maximum);
    }

    bool isProgressiveCarryCandidate(
        const ActionCandidate& candidate,
        PitchPoint carrierPosition,
        AttackingDirection direction) {
        return (candidate.type == BallCarrierActionType::Carry
            || candidate.type == BallCarrierActionType::Dribble
            || candidate.type == BallCarrierActionType::CutInside)
            && ((candidate.intendedTarget.x - carrierPosition.x) * directionSign(direction)) > 8.0;
    }

    void applyPossessionUrgency(
        std::vector<ActionCandidate>& candidates,
        PitchPoint carrierPosition,
        AttackingDirection direction,
        double urgency) {
        if (urgency <= 0.0) {
            return;
        }

        for (ActionCandidate& candidate : candidates) {
            double adjustment = 0.0;
            switch (candidate.type) {
            case BallCarrierActionType::Hold:
                adjustment = -16.0 * urgency;
                break;
            case BallCarrierActionType::BackPass:
                adjustment = -16.0 * urgency;
                break;
            case BallCarrierActionType::ShortPass:
                adjustment = (((candidate.intendedTarget.x - carrierPosition.x) * directionSign(direction)) > 5.0
                    ? 8.0
                    : -16.0) * urgency;
                break;
            case BallCarrierActionType::SwitchPlay:
                adjustment = 8.0 * urgency;
                break;
            case BallCarrierActionType::ThroughBall:
                adjustment = 11.0 * urgency;
                break;
            case BallCarrierActionType::LowCross:
            case BallCarrierActionType::HighCross:
            case BallCarrierActionType::Cutback:
                adjustment = 10.0 * urgency;
                break;
            case BallCarrierActionType::Carry:
                adjustment = (isProgressiveCarryCandidate(candidate, carrierPosition, direction) ? 8.0 : -12.0)
                    * urgency;
                break;
            case BallCarrierActionType::Dribble:
            case BallCarrierActionType::CutInside:
                adjustment = 7.0 * urgency;
                break;
            case BallCarrierActionType::Shoot:
                adjustment = 0.0;
                break;
            case BallCarrierActionType::Clear:
                break;
            }
            if (urgency >= 0.90
                && (candidate.type == BallCarrierActionType::Hold
                    || candidate.type == BallCarrierActionType::BackPass
                    || (candidate.type == BallCarrierActionType::ShortPass
                        && ((candidate.intendedTarget.x - carrierPosition.x) * directionSign(direction)) <= 5.0)
                    || (candidate.type == BallCarrierActionType::Carry
                        && !isProgressiveCarryCandidate(candidate, carrierPosition, direction)))) {
                adjustment -= 8.0 * urgency;
            }
            candidate.finalScore = contractClampFor(candidate.type, candidate.finalScore + adjustment);
        }
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
        double selectionScore = 0.0;
        switch (option.kind) {
        case PassOptionKind::SafePass:
        case PassOptionKind::BackPass:
            selectionScore = normalizeOptionScore(option.score, 10.0, 0.52, 18.0, 58.0);
            break;
        case PassOptionKind::ProgressivePass:
        case PassOptionKind::SwitchPlay:
            selectionScore = normalizeOptionScore(option.score, 11.0, 0.58, 18.0, 65.0);
            break;
        case PassOptionKind::ThroughBall:
        case PassOptionKind::Cross:
        case PassOptionKind::Cutback:
            selectionScore = normalizeOptionScore(option.score, 10.0, 0.62, 16.0, 70.0);
            break;
        }
        candidate.finalScore = selectionScore;
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
        switch (option.kind) {
        case CarryOptionKind::SafeCarry:
            candidate.finalScore = normalizeOptionScore(option.score, 8.0, 0.42, 16.0, 45.0);
            break;
        case CarryOptionKind::ProgressiveCarry:
            candidate.finalScore = normalizeOptionScore(option.score, 8.0, 0.52, 16.0, 60.0);
            break;
        case CarryOptionKind::Dribble:
            candidate.finalScore = normalizeOptionScore(option.score, 7.0, 0.50, 12.0, 55.0);
            break;
        }
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
        candidate.finalScore = clampCandidateScore(option.score, 0.0, 52.0);
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
        18.0 + mentalitySafeBonus(tacticalSetup.mentality) * 0.5,
        controlledBall ? 12.0 : 4.0,
        8.0,
        pressurePenalty * 0.45));
    candidates.back().finalScore = clampCandidateScore(candidates.back().finalScore, 12.0, 42.0);

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
                candidate.finalScore = clampScore(candidate.finalScore + 4.0);
            }
        }
    }

    applyPossessionUrgency(
        candidates,
        carrierPosition,
        direction,
        progressionUrgency(request.simulationState, tacticalSetup));

    return candidates;
}
