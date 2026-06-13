#include"fm/match_engine/phase/SpaceContext.h"

#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<algorithm>
#include<cmath>

namespace {
    AttackingDirection directionFor(const TeamSimState& team) {
        return team.side == TeamSide::Home
            ? AttackingDirection::HomeToAway
            : AttackingDirection::AwayToHome;
    }

    double progressFor(PitchPoint point, AttackingDirection direction) {
        const double progress = direction == AttackingDirection::HomeToAway
            ? point.x / PitchGeometry::LengthMeters
            : (PitchGeometry::LengthMeters - point.x) / PitchGeometry::LengthMeters;
        return std::clamp(progress, 0.0, 1.0);
    }

    double forwardMeters(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? to.x - from.x
            : from.x - to.x;
    }

    bool aheadOfBall(PitchPoint player, PitchPoint ball, AttackingDirection direction) {
        return forwardMeters(ball, player, direction) > 1.0;
    }

    bool behindBall(PitchPoint player, PitchPoint ball, AttackingDirection direction) {
        return forwardMeters(player, ball, direction) > -1.0;
    }

    BallZone zoneFor(double progress) {
        if (progress < 0.34) {
            return BallZone::DefensiveThird;
        }
        if (progress < 0.50) {
            return BallZone::MiddleThird;
        }
        if (progress < 0.66) {
            return BallZone::AttackingHalf;
        }
        return BallZone::FinalThird;
    }

    BallFlank flankFor(PitchPoint point) {
        const double third = PitchGeometry::WidthMeters / 3.0;
        if (point.y < third) {
            return BallFlank::Left;
        }
        if (point.y > third * 2.0) {
            return BallFlank::Right;
        }
        return BallFlank::Center;
    }

    int opponentsInLane(
        const std::vector<PlayerSimState>& opponents,
        PitchPoint ball,
        AttackingDirection direction,
        double forwardDepth,
        double halfWidth) {
        int count = 0;
        for (const PlayerSimState& opponent : opponents) {
            const double forward = forwardMeters(ball, opponent.position, direction);
            if (forward > 0.0
                && forward <= forwardDepth
                && std::abs(opponent.position.y - ball.y) <= halfWidth) {
                ++count;
            }
        }
        return count;
    }

    int opponentsInWideChannel(
        const std::vector<PlayerSimState>& opponents,
        bool leftSide) {
        const double limit = PitchGeometry::WidthMeters / 3.0;
        int count = 0;
        for (const PlayerSimState& opponent : opponents) {
            if (leftSide && opponent.position.y < limit) {
                ++count;
            } else if (!leftSide && opponent.position.y > PitchGeometry::WidthMeters - limit) {
                ++count;
            }
        }
        return count;
    }
}

SpaceContextSnapshot SpaceContextModel::evaluate(
    const TeamSimState& team,
    const TeamSimState& opponent,
    PitchPoint ballPosition) const {
    const AttackingDirection direction = directionFor(team);

    SpaceContextSnapshot snapshot;
    snapshot.ballProgress = progressFor(ballPosition, direction);
    snapshot.ballZone = zoneFor(snapshot.ballProgress);
    snapshot.ballFlank = flankFor(ballPosition);

    for (const PlayerSimState& player : team.players) {
        if (aheadOfBall(player.position, ballPosition, direction)) {
            ++snapshot.playersAheadOfBall;
        }
        if (behindBall(player.position, ballPosition, direction)) {
            ++snapshot.playersBehindBall;
        }
        if (!player.hasBall && PitchGeometry::distance(player.position, ballPosition) <= 16.0) {
            ++snapshot.nearestSupportCount;
        }
    }
    snapshot.supportAvailableNearBall = snapshot.nearestSupportCount >= 2;

    const int centralLaneOpponents = opponentsInLane(opponent.players, ballPosition, direction, 30.0, 8.0);
    const int broadLaneOpponents = opponentsInLane(opponent.players, ballPosition, direction, 36.0, 18.0);
    snapshot.openForwardLaneScore = std::clamp(
        82.0
            - static_cast<double>(centralLaneOpponents) * 18.0
            - static_cast<double>(broadLaneOpponents) * 6.0
            + static_cast<double>(snapshot.playersAheadOfBall) * 4.0,
        0.0,
        100.0);

    const int leftWideOpponents = opponentsInWideChannel(opponent.players, true);
    const int rightWideOpponents = opponentsInWideChannel(opponent.players, false);
    const double leftWideScore = std::clamp(76.0 - static_cast<double>(leftWideOpponents) * 11.0, 0.0, 100.0);
    const double rightWideScore = std::clamp(76.0 - static_cast<double>(rightWideOpponents) * 11.0, 0.0, 100.0);
    snapshot.openWideLaneScore = std::max(leftWideScore, rightWideScore);
    snapshot.wideSpaceAvailableLeft = leftWideScore >= 48.0;
    snapshot.wideSpaceAvailableRight = rightWideScore >= 48.0;
    snapshot.centralSpaceAvailable = centralLaneOpponents <= 1 || snapshot.openForwardLaneScore >= 56.0;

    snapshot.counterOpportunityScore = std::clamp(
        snapshot.openForwardLaneScore * 0.62
            + static_cast<double>(snapshot.playersAheadOfBall) * 6.0
            + (snapshot.ballProgress >= 0.42 ? 10.0 : 0.0)
            - static_cast<double>(centralLaneOpponents) * 4.0,
        0.0,
        100.0);
    snapshot.counterThreatScore = snapshot.counterOpportunityScore;
    snapshot.restDefenseStable = snapshot.playersBehindBall >= 6
        && snapshot.openForwardLaneScore < 62.0;

    return snapshot;
}
