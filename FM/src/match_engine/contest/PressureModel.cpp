#include"fm/match_engine/contest/PressureContext.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<limits>

namespace {
    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double centralGoalChannelShare(PitchPoint position) {
        const double distanceFromCenter =
            std::abs(position.y - PitchGeometry::WidthMeters / 2.0);
        return std::clamp(1.0 - (distanceFromCenter / (PitchGeometry::WidthMeters * 0.24)), 0.0, 1.0);
    }

    double ballProgression(PitchPoint point, AttackingDirection direction) {
        const double xProgress = direction == AttackingDirection::HomeToAway
            ? point.x / PitchGeometry::LengthMeters
            : (PitchGeometry::LengthMeters - point.x) / PitchGeometry::LengthMeters;
        return std::clamp(xProgress, 0.0, 1.0);
    }

    double distancePointToSegment(PitchPoint point, PitchPoint start, PitchPoint end) {
        const double vx = end.x - start.x;
        const double vy = end.y - start.y;
        const double wx = point.x - start.x;
        const double wy = point.y - start.y;
        const double lengthSquared = vx * vx + vy * vy;
        if (lengthSquared <= 0.001) {
            return PitchGeometry::distance(point, start);
        }

        const double t = std::clamp(((wx * vx) + (wy * vy)) / lengthSquared, 0.0, 1.0);
        return PitchGeometry::distance(point, PitchPoint{ start.x + t * vx, start.y + t * vy });
    }

    double angleBetweenDegrees(PitchPoint origin, PitchPoint a, PitchPoint b) {
        const double ax = a.x - origin.x;
        const double ay = a.y - origin.y;
        const double bx = b.x - origin.x;
        const double by = b.y - origin.y;
        const double aLength = std::sqrt(ax * ax + ay * ay);
        const double bLength = std::sqrt(bx * bx + by * by);
        if (aLength <= 0.001 || bLength <= 0.001) {
            return 180.0;
        }

        const double cosine = clampDouble(((ax * bx) + (ay * by)) / (aLength * bLength), -1.0, 1.0);
        return std::acos(cosine) * 180.0 / 3.14159265358979323846;
    }

    bool pointBetweenWithinLane(
        PitchPoint point,
        PitchPoint start,
        PitchPoint end,
        double laneWidthMeters) {
        const double vx = end.x - start.x;
        const double vy = end.y - start.y;
        const double wx = point.x - start.x;
        const double wy = point.y - start.y;
        const double lengthSquared = vx * vx + vy * vy;
        if (lengthSquared <= 0.001) {
            return PitchGeometry::distance(point, start) <= laneWidthMeters;
        }

        const double t = ((wx * vx) + (wy * vy)) / lengthSquared;
        return t >= 0.05
            && t <= 1.05
            && distancePointToSegment(point, start, end) <= laneWidthMeters;
    }

    double intentPressureContribution(PlayerIntentType intent) {
        switch (intent) {
        case PlayerIntentType::PressBallCarrier:
        case PlayerIntentType::AttemptTackle:
            return 18.0;
        case PlayerIntentType::ContainBallCarrier:
            return 13.0;
        case PlayerIntentType::BlockPassingLane:
        case PlayerIntentType::InterceptBallPath:
            return 15.0;
        case PlayerIntentType::MarkOpponent:
        case PlayerIntentType::CoverSpace:
            return 9.0;
        case PlayerIntentType::None:
        case PlayerIntentType::HoldAttackingShape:
        case PlayerIntentType::MoveToSupport:
        case PlayerIntentType::DropForPass:
        case PlayerIntentType::MakeRunBehind:
        case PlayerIntentType::AttackNearPost:
        case PlayerIntentType::AttackFarPost:
        case PlayerIntentType::AttackCutbackZone:
        case PlayerIntentType::ReceivePass:
        case PlayerIntentType::OccupyWidth:
        case PlayerIntentType::OccupyHalfSpace:
        case PlayerIntentType::HoldDefensiveShape:
        case PlayerIntentType::RecoverToGoal:
        case PlayerIntentType::ProtectGoalZone:
        case PlayerIntentType::RecoverLooseBall:
        case PlayerIntentType::ClearDanger:
            return 0.0;
        }
        return 0.0;
    }

    PressureDangerZone dangerZoneFor(
        PitchPoint point,
        PitchPoint opponentGoalCenter,
        AttackingDirection direction) {
        const double progress = ballProgression(point, direction);
        const double goalDistance = PitchGeometry::distance(point, opponentGoalCenter);
        const double centralShare = centralGoalChannelShare(point);
        if (goalDistance <= 5.5 && centralShare >= 0.35) {
            return PressureDangerZone::Goalmouth;
        }
        if (progress >= 0.84 && centralShare >= 0.45) {
            return PressureDangerZone::CentralBox;
        }
        if (progress >= 0.78) {
            return PressureDangerZone::Box;
        }
        if (progress >= 0.66) {
            return PressureDangerZone::FinalThird;
        }
        return PressureDangerZone::Midfield;
    }
}

double zonePressureBonus(PressureDangerZone zone) {
    switch (zone) {
    case PressureDangerZone::Midfield:
        return 0.0;
    case PressureDangerZone::FinalThird:
        return 5.0;
    case PressureDangerZone::Box:
        return 10.0;
    case PressureDangerZone::CentralBox:
        return 15.0;
    case PressureDangerZone::Goalmouth:
        return 22.0;
    }
    return 0.0;
}

double contestDistanceLimit(PressureDangerZone zone, BallCarrierActionType actionType) {
    double limit = 2.45;
    if (actionType == BallCarrierActionType::Carry) {
        limit = 1.45;
    } else if (actionType == BallCarrierActionType::Dribble
        || actionType == BallCarrierActionType::CutInside) {
        limit = 2.05;
    }
    switch (zone) {
    case PressureDangerZone::Midfield:
        break;
    case PressureDangerZone::FinalThird:
        limit += 0.25;
        break;
    case PressureDangerZone::Box:
        limit += 0.55;
        break;
    case PressureDangerZone::CentralBox:
        limit += 0.85;
        break;
    case PressureDangerZone::Goalmouth:
        limit += 1.10;
        break;
    }
    return limit;
}

bool isRealContest(const PressureContext& pressure, BallCarrierActionType actionType) {
    if (pressure.closestOutfieldDefenderId == 0
        || pressure.closestOutfieldDefenderDistance < 0.0) {
        return false;
    }

    const double limit = contestDistanceLimit(pressure.dangerZone, actionType);
    return pressure.closestOutfieldDefenderDistance <= limit
        || (pressure.defenderBetweenBallAndPath && pressure.closestOutfieldDefenderDistance <= limit + 1.35)
        || (pressure.defenderBetweenBallAndGoal
            && pressure.pressureStrength >= 24.0
            && pressure.closestOutfieldDefenderDistance <= limit + 1.0)
        || (pressure.dangerZone == PressureDangerZone::Goalmouth
            && pressure.closestOutfieldDefenderDistance <= 6.0);
}

PressureContext PressureModel::build(const PressureContextRequest& request) const {
    PressureContext context;
    context.dangerZone = dangerZoneFor(
        request.ballPosition,
        request.opponentGoalCenter,
        request.direction);

    double closestDistance = std::numeric_limits<double>::max();
    for (const PressurePlayer& defender : request.defenders) {
        const double distance = PitchGeometry::distance(defender.position, request.ballPosition);
        if (context.closestOutfieldDefenderId == 0 || distance < closestDistance) {
            context.closestOutfieldDefenderId = defender.playerId;
            context.closestOutfieldDefenderTeamId = defender.teamId;
            context.closestOutfieldDefenderPosition = defender.position;
            closestDistance = distance;
        }
    }
    if (context.closestOutfieldDefenderId == 0) {
        return context;
    }
    context.closestOutfieldDefenderDistance = closestDistance;

    context.defenderAngleToGoalDegrees =
        angleBetweenDegrees(request.ballPosition, request.opponentGoalCenter, context.closestOutfieldDefenderPosition);
    context.defenderBetweenBallAndGoal = pointBetweenWithinLane(
        context.closestOutfieldDefenderPosition,
        request.ballPosition,
        request.opponentGoalCenter,
        4.5);
    context.defenderBetweenBallAndPath = pointBetweenWithinLane(
        context.closestOutfieldDefenderPosition,
        request.ballPosition,
        request.intendedTarget,
        request.actionType == BallCarrierActionType::Carry ? 3.0 : 3.6);

    double intentPressure = 0.0;
    double nearbyPressure = 0.0;
    for (const PressurePlayer& defender : request.defenders) {
        const double distance = PitchGeometry::distance(defender.position, request.ballPosition);
        if (defender.playerId != context.closestOutfieldDefenderId && distance <= 5.5) {
            ++context.supportDefendersNearby;
        }
        if (distance <= 3.0) {
            nearbyPressure += 28.0;
        } else if (distance <= 6.0) {
            nearbyPressure += 15.0;
        } else if (distance <= 10.0) {
            nearbyPressure += 6.0;
        }

        const double intent = intentPressureContribution(defender.intentType);
        if (intent > 0.0) {
            const double laneDistance =
                distancePointToSegment(defender.position, request.ballPosition, request.intendedTarget);
            if (distance <= 7.0 || laneDistance <= 5.0) {
                intentPressure += intent * (distance <= 4.0 ? 1.0 : 0.65);
            }
        }
    }

    const double laneBonus = context.defenderBetweenBallAndPath ? 12.0 : 0.0;
    const double goalSideBonus = context.defenderBetweenBallAndGoal ? 8.0 : 0.0;
    const double supportBonus = std::min(12.0, static_cast<double>(context.supportDefendersNearby) * 4.0);
    context.pressureStrength = clampDouble(
        nearbyPressure
            + intentPressure
            + laneBonus
            + goalSideBonus
            + supportBonus
            + zonePressureBonus(context.dangerZone),
        0.0,
        100.0);
    return context;
}
