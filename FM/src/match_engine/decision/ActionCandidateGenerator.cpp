#include"fm/match_engine/decision/ActionCandidateGenerator.h"

#include"fm/match_engine/decision/PassOptionEvaluator.h"

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

    PitchPoint goalCenterFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    bool isWide(PitchPoint point) {
        return point.y <= PitchGeometry::WidthMeters * 0.22
            || point.y >= PitchGeometry::WidthMeters * 0.78;
    }

    bool isNearOpponentBox(PitchPoint point, AttackingDirection direction) {
        if (direction == AttackingDirection::HomeToAway) {
            return point.x >= PitchGeometry::LengthMeters - 24.0
                || PitchGeometry::isInsideAwayPenaltyArea(point);
        }

        return point.x <= 24.0 || PitchGeometry::isInsideHomePenaltyArea(point);
    }

    double distanceToOpponentGoal(PitchPoint point, AttackingDirection direction) {
        return PitchGeometry::distance(point, goalCenterFor(direction));
    }

    double centrality(PitchPoint point) {
        return 1.0 - std::min(
            1.0,
            std::abs(point.y - (PitchGeometry::WidthMeters / 2.0))
                / (PitchGeometry::WidthMeters / 2.0));
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

    double mentalityAttackBonus(TeamMentality mentality) {
        return mentality == TeamMentality::Attacking ? 10.0 : 0.0;
    }

    double directProgressionBonus(const TacticalSetup& tacticalSetup) {
        double bonus = 0.0;
        if (tacticalSetup.tempo == TeamTempo::High) {
            bonus += 5.0;
        }
        if (tacticalSetup.passingDirectness == PassingDirectness::Direct) {
            bonus += 5.0;
        }

        return bonus;
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
            assignedRoleFor(request.teamSnapshot, request.ballCarrier.playerId),
            tacticalSetup,
            carrierPosition,
            direction,
            pressurePenalty
        });
    for (const PassOption& option : passOptions) {
        candidates.push_back(buildCandidateFromPassOption(option));
    }

    candidates.push_back(buildCandidate(
        BallCarrierActionType::Carry,
        advanceTarget(carrierPosition, direction, 12.0),
        0,
        26.0 + directProgressionBonus(tacticalSetup),
        18.0,
        8.0,
        pressurePenalty * 0.7));

    if (isNearOpponentBox(carrierPosition, direction)) {
        const double goalDistance = distanceToOpponentGoal(carrierPosition, direction);
        const double central = centrality(carrierPosition);
        const bool goodAngle = central >= 0.42 && goalDistance <= 24.0;
        const bool lowPressure = pressurePenalty <= 12.0;
        const bool attackingIntent = tacticalSetup.mentality == TeamMentality::Attacking;
        const bool wideLowValue = isWide(carrierPosition) && central < 0.30 && goalDistance > 11.0;
        if ((goodAngle || lowPressure || attackingIntent) && !(wideLowValue && !attackingIntent)) {
            const double contextScore = 14.0
                + std::max(0.0, 24.0 - goalDistance) * 0.55
                + central * 7.0
                + (lowPressure ? 4.0 : 0.0);
            const double tacticalScore = 18.0 + mentalityAttackBonus(tacticalSetup.mentality);
            const double widePenalty = wideLowValue ? 7.0 : 0.0;
            candidates.push_back(buildCandidate(
                BallCarrierActionType::Shoot,
                goalCenterFor(direction),
                0,
                tacticalScore,
                contextScore,
                7.0,
                pressurePenalty * 0.65 + widePenalty));
        }
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
