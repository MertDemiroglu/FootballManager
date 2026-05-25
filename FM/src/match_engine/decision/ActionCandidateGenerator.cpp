#include"fm/match_engine/decision/ActionCandidateGenerator.h"

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

    double distancePointToSegment(PitchPoint point, PitchPoint a, PitchPoint b) {
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double lengthSquared = dx * dx + dy * dy;
        if (lengthSquared <= 0.001) {
            return PitchGeometry::distance(point, a);
        }

        const double t = std::clamp(
            (((point.x - a.x) * dx) + ((point.y - a.y) * dy)) / lengthSquared,
            0.0,
            1.0);
        return PitchGeometry::distance(point, PitchPoint{ a.x + dx * t, a.y + dy * t });
    }

    double laneRisk(
        PitchPoint from,
        PitchPoint to,
        const std::vector<PlayerSimState>& opponents) {
        double risk = 0.0;
        for (const PlayerSimState& opponent : opponents) {
            const double pathDistance = distancePointToSegment(opponent.position, from, to);
            if (pathDistance <= 3.0) {
                risk += 18.0;
            } else if (pathDistance <= 5.0) {
                risk += 9.0;
            } else if (pathDistance <= 8.0) {
                risk += 3.0;
            }
        }
        return std::min(32.0, risk);
    }

    std::vector<const PlayerSimState*> rankedTeammates(
        const ActionCandidateGenerationRequest& request,
        bool requireBehind,
        bool requireAhead,
        std::size_t limit) {
        std::vector<const PlayerSimState*> candidates;
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
            candidates.push_back(&teammate);
        }

        std::sort(
            candidates.begin(),
            candidates.end(),
            [&request](const PlayerSimState* lhs, const PlayerSimState* rhs) {
                return PitchGeometry::distance(request.ballCarrier.position, lhs->position)
                    < PitchGeometry::distance(request.ballCarrier.position, rhs->position);
            });

        if (candidates.size() > limit) {
            candidates.resize(limit);
        }
        return candidates;
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

    double shortPassingBonus(const TacticalSetup& tacticalSetup) {
        double bonus = 0.0;
        if (tacticalSetup.passingDirectness == PassingDirectness::Short) {
            bonus += 8.0;
        }
        if (tacticalSetup.tempo == TeamTempo::Low) {
            bonus += 4.0;
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
        const double risk = laneRisk(carrierPosition, backTarget->position, request.opponents);
        candidates.push_back(buildCandidate(
            BallCarrierActionType::BackPass,
            backTarget->position,
            backTarget->playerId,
            34.0 + mentalitySafeBonus(tacticalSetup.mentality) + shortPassingBonus(tacticalSetup),
            18.0,
            10.0,
            pressurePenalty * 0.25 + risk * 0.35));
    }

    const std::vector<const PlayerSimState*> supportTargets =
        rankedTeammates(request, false, false, tacticalSetup.passingDirectness == PassingDirectness::Short ? 4 : 3);
    for (const PlayerSimState* shortTarget : supportTargets) {
        const double targetDistance = PitchGeometry::distance(carrierPosition, shortTarget->position);
        const double risk = laneRisk(carrierPosition, shortTarget->position, request.opponents);
        const bool ahead = isAhead(carrierPosition, shortTarget->position, direction);
        if (targetDistance > 26.0 && tacticalSetup.passingDirectness != PassingDirectness::Direct) {
            continue;
        }
        candidates.push_back(buildCandidate(
            BallCarrierActionType::ShortPass,
            shortTarget->position,
            shortTarget->playerId,
            30.0 + shortPassingBonus(tacticalSetup),
            ahead ? 19.0 : 16.0,
            std::clamp(14.0 - targetDistance * 0.10, 7.0, 14.0),
            pressurePenalty * 0.35 + risk * 0.55));
    }

    const std::vector<const PlayerSimState*> progressiveTargets =
        rankedTeammates(request, false, true, tacticalSetup.passingDirectness == PassingDirectness::Direct ? 3 : 2);
    for (const PlayerSimState* target : progressiveTargets) {
        const double targetDistance = PitchGeometry::distance(carrierPosition, target->position);
        if (targetDistance < 10.0 || targetDistance > 34.0) {
            continue;
        }
        const double risk = laneRisk(carrierPosition, target->position, request.opponents);
        const bool direct = tacticalSetup.passingDirectness == PassingDirectness::Direct;
        candidates.push_back(buildCandidate(
            direct ? BallCarrierActionType::ThroughBall : BallCarrierActionType::ShortPass,
            target->position,
            target->playerId,
            direct ? 26.0 + directProgressionBonus(tacticalSetup) : 24.0,
            18.0 + std::min(8.0, targetDistance * 0.18),
            9.0,
            pressurePenalty * 0.45 + risk * (direct ? 0.65 : 0.55)));
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
