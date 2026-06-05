#include"fm/match_engine/ball/ShotBlockResolver.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/ball/BallTrajectoryBuilder.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<cmath>

namespace {
    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double attributeOrDefault(int value) {
        const ShootingModelTuning tuning;
        return value > 0 ? clampDouble(static_cast<double>(value), 0.0, 100.0) : tuning.defaultAttribute;
    }

    double defenderBlockSkill(const ShotBlocker& blocker) {
        return clampDouble(
            attributeOrDefault(blocker.attributes.mental.positioning) * 0.28
                + attributeOrDefault(blocker.attributes.mental.concentration) * 0.22
                + attributeOrDefault(blocker.attributes.technical.marking) * 0.18
                + attributeOrDefault(blocker.attributes.technical.tackling) * 0.16
                + attributeOrDefault(blocker.attributes.physical.agility) * 0.10
                + clampDouble(static_cast<double>(blocker.baseOverall), 0.0, 100.0) * 0.06,
            0.0,
            100.0);
    }

    double distancePointToSegment(PitchPoint point, PitchPoint start, PitchPoint end, PitchPoint& closest) {
        const double dx = end.x - start.x;
        const double dy = end.y - start.y;
        const double lengthSquared = dx * dx + dy * dy;
        if (lengthSquared <= 0.0001) {
            closest = start;
            return PitchGeometry::distance(point, start);
        }

        const double t = clampDouble(
            ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared,
            0.0,
            1.0);
        closest = PitchPoint{ start.x + dx * t, start.y + dy * t };
        return PitchGeometry::distance(point, closest);
    }
}

ShotBlockResult ShotBlockResolver::resolve(const ShotBlockRequest& request) const {
    const ShootingModelTuning tuning;
    ShotBlockResult best;
    double bestProbability = 0.0;

    for (const ShotBlocker& blocker : request.blockers) {
        if (blocker.playerId == 0 || blocker.teamId == 0) {
            continue;
        }

        PitchPoint contactPoint;
        const double laneDistance = distancePointToSegment(
            blocker.position,
            request.trajectory.start,
            request.trajectory.actualTarget,
            contactPoint);
        if (laneDistance > tuning.blockLaneWidthMeters + 2.5) {
            continue;
        }

        const double ballDistance = PitchGeometry::distance(request.trajectory.start, contactPoint);
        const double ballArrival = request.trajectory.startSecond
            + (ballDistance / std::max(request.trajectory.speedMetersPerSecond, 1.0));
        const double defenderDistance = PitchGeometry::distance(blocker.position, contactPoint);
        const double reactionWindow =
            tuning.blockReactionWindowSeconds
            + clampDouble((defenderBlockSkill(blocker) - 50.0) / 100.0, -0.12, 0.24);
        const double playerArrival =
            request.trajectory.startSecond + (defenderDistance / 6.5);
        const double arrivalMargin = playerArrival - ballArrival;
        if (arrivalMargin > reactionWindow) {
            continue;
        }

        const double laneFactor = clampDouble(1.0 - (laneDistance / (tuning.blockLaneWidthMeters + 2.5)), 0.0, 1.0);
        const double timingFactor = clampDouble(1.0 - std::max(0.0, arrivalMargin) / std::max(reactionWindow, 0.05), 0.0, 1.0);
        const double skillFactor = defenderBlockSkill(blocker) / 100.0;
        const double speedPenalty = clampDouble((request.execution.shotPower - 18.0) / 26.0, 0.0, 0.45);
        const double probability = clampDouble(
            tuning.baseBlockProbability
                + request.quality.blockRisk * 0.44
                + laneFactor * 0.20
                + timingFactor * 0.15
                + skillFactor * 0.12
                + (request.context.lanePressure / 100.0) * 0.10
                - speedPenalty * 0.18,
            0.0,
            tuning.maximumBlockProbability);

        const double roll = matchEngineDeterministicUnitInterval(
            request.seed
                ^ matchEngineMix64(static_cast<std::uint64_t>(blocker.playerId))
                ^ 0xb10cULL);
        if (roll < probability && probability > bestProbability) {
            best.blocked = true;
            best.blockerPlayerId = blocker.playerId;
            best.blockerTeamId = blocker.teamId;
            best.contactPoint = PitchGeometry::clampToPitch(contactPoint);
            best.deflectionStrength = clampDouble(
                0.35 + (request.execution.shotPower - 18.0) / 30.0 + laneFactor * 0.20,
                0.25,
                0.95);
            bestProbability = probability;
        }
    }

    return best;
}
