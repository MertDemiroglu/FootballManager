#include"fm/match_engine/ActionCandidateGenerator.h"

#include<algorithm>
#include<limits>

namespace {
    double clampScore(double value) {
        return std::clamp(value, 0.0, 100.0);
    }

    double directionSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    bool isAhead(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return (to.x - from.x) * directionSign(direction) > 1.0;
    }

    bool isBehind(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return (to.x - from.x) * directionSign(direction) < -1.0;
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

    PitchPoint lowCrossTarget(AttackingDirection direction) {
        const double x = direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters - 11.0
            : 11.0;

        return PitchPoint{ x, PitchGeometry::WidthMeters / 2.0 };
    }

    bool isWide(PitchPoint point) {
        return point.y <= PitchGeometry::WidthMeters * 0.22
            || point.y >= PitchGeometry::WidthMeters * 0.78;
    }

    bool isFinalThird(PitchPoint point, AttackingDirection direction) {
        if (direction == AttackingDirection::HomeToAway) {
            return point.x >= PitchGeometry::LengthMeters * 0.66;
        }

        return point.x <= PitchGeometry::LengthMeters * 0.34;
    }

    bool isNearOpponentBox(PitchPoint point, AttackingDirection direction) {
        if (direction == AttackingDirection::HomeToAway) {
            return point.x >= PitchGeometry::LengthMeters - 24.0
                || PitchGeometry::isInsideAwayPenaltyArea(point);
        }

        return point.x <= 24.0 || PitchGeometry::isInsideHomePenaltyArea(point);
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

    const PlayerSimState* nearestTeammate(
        const ActionCandidateGenerationRequest& request,
        bool requireBehind,
        bool requireAhead) {
        const PlayerSimState* best = nullptr;
        double bestDistance = std::numeric_limits<double>::max();

        for (const PlayerSimState& teammate : request.teammates) {
            if (teammate.playerId == request.ballCarrier.playerId) {
                continue;
            }

            if (requireBehind
                && !isBehind(
                    request.ballCarrier.position,
                    teammate.position,
                    request.teamShapeContext.attackingDirection)) {
                continue;
            }

            if (requireAhead
                && !isAhead(
                    request.ballCarrier.position,
                    teammate.position,
                    request.teamShapeContext.attackingDirection)) {
                continue;
            }

            const double distance =
                PitchGeometry::distance(request.ballCarrier.position, teammate.position);
            if (distance < bestDistance) {
                best = &teammate;
                bestDistance = distance;
            }
        }

        return best;
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
}

std::vector<ActionCandidate> ActionCandidateGenerator::generate(
    const ActionCandidateGenerationRequest& request) const {
    std::vector<ActionCandidate> candidates;

    const TacticalSetup& tacticalSetup = request.teamShapeContext.tacticalSetup;
    const AttackingDirection direction = request.teamShapeContext.attackingDirection;
    const PitchPoint carrierPosition = request.ballCarrier.position;
    const double pressurePenalty = nearestOpponentPressure(carrierPosition, request.opponents);
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

    if (const PlayerSimState* backTarget = nearestTeammate(request, true, false)) {
        candidates.push_back(buildCandidate(
            BallCarrierActionType::BackPass,
            backTarget->position,
            backTarget->playerId,
            30.0 + mentalitySafeBonus(tacticalSetup.mentality),
            18.0,
            10.0,
            pressurePenalty * 0.35));
    }

    if (const PlayerSimState* shortTarget = nearestTeammate(request, false, false)) {
        candidates.push_back(buildCandidate(
            BallCarrierActionType::ShortPass,
            shortTarget->position,
            shortTarget->playerId,
            28.0,
            isAhead(carrierPosition, shortTarget->position, direction) ? 20.0 : 12.0,
            10.0,
            pressurePenalty * 0.4));
    }

    candidates.push_back(buildCandidate(
        BallCarrierActionType::Carry,
        advanceTarget(carrierPosition, direction, 12.0),
        0,
        26.0 + directProgressionBonus(tacticalSetup),
        18.0,
        8.0,
        pressurePenalty * 0.7));

    if (isWide(carrierPosition) && isFinalThird(carrierPosition, direction)) {
        candidates.push_back(buildCandidate(
            BallCarrierActionType::LowCross,
            lowCrossTarget(direction),
            0,
            28.0 + mentalityAttackBonus(tacticalSetup.mentality),
            tacticalSetup.width == TeamWidth::Wide ? 22.0 : 16.0,
            10.0,
            pressurePenalty * 0.55));
    }

    if (isNearOpponentBox(carrierPosition, direction)) {
        candidates.push_back(buildCandidate(
            BallCarrierActionType::Shoot,
            goalCenterFor(direction),
            0,
            24.0 + mentalityAttackBonus(tacticalSetup.mentality),
            26.0,
            10.0,
            pressurePenalty * 0.45));
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
