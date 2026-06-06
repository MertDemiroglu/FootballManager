#include"fm/match_engine/ball/ShotContextBuilder.h"

#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<cmath>

namespace {
    constexpr double Pi = 3.14159265358979323846;

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    PitchPoint goalCenterFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    double goalLineXFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters
            : 0.0;
    }

    double distancePointToSegment(
        PitchPoint point,
        PitchPoint start,
        PitchPoint end,
        const ShotContextTuning& tuning) {
        const double dx = end.x - start.x;
        const double dy = end.y - start.y;
        const double lengthSquared = dx * dx + dy * dy;
        if (lengthSquared <= tuning.minimumSegmentLengthSquared) {
            return PitchGeometry::distance(point, start);
        }

        const double t = clampDouble(
            ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared,
            0.0,
            1.0);
        return PitchGeometry::distance(
            point,
            PitchPoint{ start.x + dx * t, start.y + dy * t });
    }

    double goalAngleRadians(PitchPoint shotLocation, AttackingDirection direction) {
        const double halfGoalWidth = PitchGeometry::GoalWidthMeters / 2.0;
        const double centerY = PitchGeometry::WidthMeters / 2.0;
        const double goalX = goalLineXFor(direction);
        const PitchPoint postA{ goalX, centerY - halfGoalWidth };
        const PitchPoint postB{ goalX, centerY + halfGoalWidth };
        const double ax = postA.x - shotLocation.x;
        const double ay = postA.y - shotLocation.y;
        const double bx = postB.x - shotLocation.x;
        const double by = postB.y - shotLocation.y;
        const double lenA = std::hypot(ax, ay);
        const double lenB = std::hypot(bx, by);
        if (lenA <= 0.001 || lenB <= 0.001) {
            return Pi;
        }
        return std::acos(clampDouble(((ax * bx) + (ay * by)) / (lenA * lenB), -1.0, 1.0));
    }
}

ShotContext ShotContextBuilder::build(const ShotContextBuildRequest& request) const {
    const ShootingModelTuning modelTuning;
    const ShotContextTuning& tuning = modelTuning.context;
    ShotContext context;
    context.shotOrigin = PitchGeometry::clampToPitch(request.shotOrigin);
    context.attackingDirection = request.attackingDirection;
    context.pressure = clampDouble(request.pressure, 0.0, 100.0);
    context.shooterAttributes = request.shooterAttributes;
    context.goalkeeperAttributes = request.goalkeeperAttributes;
    context.goalkeeperStrength = clampDouble(request.goalkeeperStrength, 1.0, 100.0);
    context.seed = request.seed;

    const PitchPoint goalCenter = goalCenterFor(request.attackingDirection);
    context.distanceMeters = PitchGeometry::distance(context.shotOrigin, goalCenter);
    context.angleRadians = goalAngleRadians(context.shotOrigin, request.attackingDirection);
    context.centrality = clampDouble(context.angleRadians / Pi, 0.0, 1.0);

    context.nearestDefenderDistance = tuning.fallbackNearestDefenderDistance;
    double lanePressure = 0.0;
    for (PitchPoint defenderPosition : request.defenderPositions) {
        const double defenderDistance = PitchGeometry::distance(context.shotOrigin, defenderPosition);
        context.nearestDefenderDistance = std::min(context.nearestDefenderDistance, defenderDistance);
        const double laneDistance =
            distancePointToSegment(defenderPosition, context.shotOrigin, goalCenter, tuning);
        const double laneWidth = tuning.blockLaneWidthMeters + tuning.lanePressureExtraWidthMeters;
        if (laneDistance <= laneWidth) {
            const double distanceFactor = clampDouble(1.0 - (laneDistance / laneWidth), 0.0, 1.0);
            const double reactionFactor = clampDouble(
                1.0 - (defenderDistance / tuning.lanePressureReactionDistanceMeters),
                0.0,
                1.0);
            lanePressure = std::max(
                lanePressure,
                (distanceFactor * tuning.lanePressureDistanceWeight)
                    + (reactionFactor * tuning.lanePressureReactionWeight));
        }
    }
    context.lanePressure = clampDouble(lanePressure, 0.0, 100.0);
    return context;
}
