#include"fm/match_engine/offball/OffBallTargetResolver.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<limits>
#include<vector>

namespace {
    double attackSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    double progress(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }

    PitchPoint goalCenter(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    double nearestDistance(
        PitchPoint point,
        const std::vector<PlayerSimState>& players,
        PlayerId excludedPlayerId = 0) {
        double nearest = std::numeric_limits<double>::max();
        for (const PlayerSimState& player : players) {
            if (player.playerId == excludedPlayerId) {
                continue;
            }
            nearest = std::min(nearest, PitchGeometry::distance(point, player.position));
        }
        return nearest == std::numeric_limits<double>::max() ? 40.0 : nearest;
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

    double laneOpenness(
        PitchPoint from,
        PitchPoint to,
        const std::vector<PlayerSimState>& opponents) {
        double risk = 0.0;
        for (const PlayerSimState& opponent : opponents) {
            const double laneDistance = distancePointToSegment(opponent.position, from, to);
            if (laneDistance <= 2.0) {
                risk += 22.0;
            } else if (laneDistance <= 4.0) {
                risk += 12.0;
            } else if (laneDistance <= 7.0) {
                risk += 5.0;
            }
        }
        return std::clamp(100.0 - risk, 0.0, 100.0);
    }

    double roleLaneScore(FormationSlotRole role, PitchPoint point) {
        const double center = PitchGeometry::WidthMeters / 2.0;
        const double fromCenter = std::abs(point.y - center);
        switch (role) {
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::LeftMidfielder:
            return point.y <= center ? 12.0 : -10.0;
        case FormationSlotRole::RightWinger:
        case FormationSlotRole::RightMidfielder:
            return point.y >= center ? 12.0 : -10.0;
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::LeftWingBack:
            return point.y <= center ? 10.0 : -8.0;
        case FormationSlotRole::RightBack:
        case FormationSlotRole::RightWingBack:
            return point.y >= center ? 10.0 : -8.0;
        case FormationSlotRole::Striker:
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::DefensiveMidfielder:
            return std::max(0.0, 14.0 - fromCenter * 0.45);
        case FormationSlotRole::Unknown:
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
            return 0.0;
        }
        return 0.0;
    }

    double offsidePenalty(
        PitchPoint point,
        PitchPoint ballPosition,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction) {
        if (opponents.size() < 2) {
            return 0.0;
        }

        std::vector<double> defenderProgress;
        defenderProgress.reserve(opponents.size());
        for (const PlayerSimState& opponent : opponents) {
            defenderProgress.push_back(progress(opponent.position, direction));
        }
        std::sort(defenderProgress.begin(), defenderProgress.end(), std::greater<double>());
        const double secondLastLine = defenderProgress[1];
        const double offsideLine = std::max(secondLastLine, progress(ballPosition, direction));
        return progress(point, direction) > offsideLine + 0.8 ? 28.0 : 0.0;
    }

    double shootingRelevance(OffBallEventType type) {
        switch (type) {
        case OffBallEventType::CutInsideRun:
        case OffBallEventType::FarPostRun:
        case OffBallEventType::NearPostRun:
        case OffBallEventType::PenaltySpotRun:
        case OffBallEventType::EdgeOfBoxSupport:
        case OffBallEventType::HalfSpaceSupport:
        case OffBallEventType::CounterForwardRun:
            return 1.0;
        case OffBallEventType::None:
        case OffBallEventType::OverlapRun:
        case OffBallEventType::UnderlapRun:
        case OffBallEventType::WideSupport:
        case OffBallEventType::BackPassSupport:
        case OffBallEventType::RestDefenseHold:
        case OffBallEventType::DefensiveRecoveryRun:
            return 0.0;
        }
        return 0.0;
    }
}

OffBallTargetResolveResult OffBallTargetResolver::resolve(
    const OffBallTargetResolveRequest& request) const {
    const SupportRegion region = clampSupportRegion(request.event.targetRegion);
    PitchPoint bestPoint = region.center();
    double bestScore = -std::numeric_limits<double>::max();

    constexpr int GridX = 5;
    constexpr int GridY = 5;
    for (int ix = 0; ix < GridX; ++ix) {
        const double xWeight = GridX == 1 ? 0.5 : static_cast<double>(ix) / (GridX - 1);
        for (int iy = 0; iy < GridY; ++iy) {
            const double yWeight = GridY == 1 ? 0.5 : static_cast<double>(iy) / (GridY - 1);
            const PitchPoint candidate = PitchGeometry::clampToPitch(PitchPoint{
                region.minX + (region.maxX - region.minX) * xWeight,
                region.minY + (region.maxY - region.minY) * yWeight
            });

            const double teammateDistance = nearestDistance(
                candidate,
                request.teammates,
                request.player.playerId);
            const double opponentDistance = nearestDistance(candidate, request.opponents);
            const double passingLane = laneOpenness(
                request.ballPosition,
                candidate,
                request.opponents);
            const double shootingLane = laneOpenness(
                candidate,
                goalCenter(request.attackingDirection),
                request.opponents);
            const double feasibilityDistance = PitchGeometry::distance(
                request.player.position,
                candidate);
            const double feasibleDistance =
                std::max(14.0, request.player.effectivePace * request.event.maxDurationSeconds * 0.95);
            const double feasibilityPenalty =
                std::max(0.0, feasibilityDistance - feasibleDistance) * 1.6;
            double score = 0.0;
            score += std::clamp(teammateDistance * 3.0, 0.0, 42.0);
            if (teammateDistance < 5.0) {
                score -= (5.0 - teammateDistance) * 9.0;
            }
            score += std::clamp(opponentDistance * 4.0, 0.0, 48.0);
            if (opponentDistance < 3.0) {
                score -= (3.0 - opponentDistance) * 14.0;
            }
            score += passingLane * 0.34;
            score += shootingLane * 0.20 * shootingRelevance(request.event.eventType);
            score += roleLaneScore(request.playerContext.role, candidate);
            score -= feasibilityPenalty;
            score -= offsidePenalty(
                candidate,
                request.ballPosition,
                request.opponents,
                request.attackingDirection);

            if (request.event.eventType == OffBallEventType::BackPassSupport
                || request.event.eventType == OffBallEventType::RestDefenseHold
                || request.event.eventType == OffBallEventType::DefensiveRecoveryRun) {
                score += std::max(
                    0.0,
                    progress(request.ballPosition, request.attackingDirection)
                        - progress(candidate, request.attackingDirection));
            } else {
                score += std::max(
                    0.0,
                    progress(candidate, request.attackingDirection)
                        - progress(request.ballPosition, request.attackingDirection)) * 0.28;
            }

            if (score > bestScore) {
                bestScore = score;
                bestPoint = candidate;
            }
        }
    }

    OffsideAwarenessResult awareness =
        OffsideAwarenessModel{}.evaluate(OffsideAwarenessRequest{
            request.player.playerId,
            request.playerContext.role,
            request.attributes,
            request.player.position,
            bestPoint,
            request.ballPosition,
            request.attackingDirection,
            request.offsideLine,
            request.currentSecond
        });
    return OffBallTargetResolveResult{ awareness.targetPoint, awareness };
}
