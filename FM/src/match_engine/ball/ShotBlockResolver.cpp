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

    double defenderBlockSkill(const ShotBlocker& blocker, const ShotBlockTuning& tuning) {
        return clampDouble(
            attributeOrDefault(blocker.attributes.mental.positioning) * tuning.defenderPositioningWeight
                + attributeOrDefault(blocker.attributes.mental.concentration) * tuning.defenderConcentrationWeight
                + attributeOrDefault(blocker.attributes.technical.marking) * tuning.defenderMarkingWeight
                + attributeOrDefault(blocker.attributes.technical.tackling) * tuning.defenderTacklingWeight
                + attributeOrDefault(blocker.attributes.physical.agility) * tuning.defenderAgilityWeight
                + clampDouble(static_cast<double>(blocker.baseOverall), 0.0, 100.0) * tuning.defenderOverallWeight,
            0.0,
            100.0);
    }

    double blockerReachHeightMeters(const ShotBlocker& blocker, const ShotBlockTuning& tuning) {
        const double jumpingReach =
            attributeOrDefault(blocker.attributes.physical.jumpingReach) / 100.0;
        return std::max(
            tuning.groundBlockReachHeightMeters,
            tuning.jumpingReachHeightBaseMeters
                + jumpingReach * tuning.jumpingReachHeightScaleMeters
                + tuning.emergencyReachMarginMeters);
    }

    double distancePointToSegment(
        PitchPoint point,
        PitchPoint start,
        PitchPoint end,
        double& segmentProgress,
        PitchPoint& closest,
        const ShotBlockTuning& tuning) {
        const double dx = end.x - start.x;
        const double dy = end.y - start.y;
        const double lengthSquared = dx * dx + dy * dy;
        if (lengthSquared <= tuning.minimumSegmentLengthSquared) {
            segmentProgress = 0.0;
            closest = start;
            return PitchGeometry::distance(point, start);
        }

        segmentProgress = clampDouble(
            ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared,
            0.0,
            1.0);
        closest = PitchPoint{ start.x + dx * segmentProgress, start.y + dy * segmentProgress };
        return PitchGeometry::distance(point, closest);
    }
}

ShotBlockResult ShotBlockResolver::resolve(const ShotBlockRequest& request) const {
    const ShootingModelTuning modelTuning;
    const ShotBlockTuning& tuning = modelTuning.block;
    ShotBlockResult best;
    double bestProbability = 0.0;

    for (const ShotBlocker& blocker : request.blockers) {
        if (blocker.playerId == 0 || blocker.teamId == 0) {
            continue;
        }

        PitchPoint contactPoint;
        double pathProgress = 0.0;
        const double laneDistance = distancePointToSegment(
            blocker.position,
            request.trajectory.start,
            request.trajectory.actualTarget,
            pathProgress,
            contactPoint,
            tuning);
        if (pathProgress < tuning.minimumBlockPathProgress
            || pathProgress > tuning.maximumBlockPathProgress) {
            continue;
        }
        if (laneDistance > tuning.blockLaneWidthMeters + tuning.extraBlockLaneWidthMeters) {
            continue;
        }

        const double ballDistance = PitchGeometry::distance(request.trajectory.start, contactPoint);
        const double ballArrival = request.trajectory.startSecond
            + (ballDistance / std::max(request.trajectory.speedMetersPerSecond, 1.0));
        const double ballHeight = ballHeightAtSecond(request.trajectory, ballArrival);
        if (ballHeight > blockerReachHeightMeters(blocker, tuning)) {
            continue;
        }
        const double defenderDistance = PitchGeometry::distance(blocker.position, contactPoint);
        const double reactionWindow =
            tuning.blockReactionWindowSeconds
            + clampDouble(
                (defenderBlockSkill(blocker, tuning) - tuning.defenderSkillBaseline)
                    / tuning.defenderSkillScale,
                tuning.minimumReactionAdjustment,
                tuning.maximumReactionAdjustment);
        const double playerArrival =
            request.trajectory.startSecond + (defenderDistance / tuning.defenderInterventionSpeed);
        const double arrivalMargin = playerArrival - ballArrival;
        if (arrivalMargin > reactionWindow) {
            continue;
        }

        const double laneFactor = clampDouble(
            1.0 - (laneDistance / (tuning.blockLaneWidthMeters + tuning.extraBlockLaneWidthMeters)),
            0.0,
            1.0);
        const double timingFactor = clampDouble(
            1.0 - std::max(0.0, arrivalMargin) / std::max(reactionWindow, tuning.timingMinimumWindowSeconds),
            0.0,
            1.0);
        const double skillFactor = defenderBlockSkill(blocker, tuning) / tuning.defenderSkillScale;
        const double speedPenalty = clampDouble(
            (request.execution.shotPower - tuning.shotPowerSpeedPenaltyBaseline)
                / tuning.shotPowerSpeedPenaltyScale,
            0.0,
            tuning.maximumSpeedPenalty);
        const double probability = clampDouble(
            tuning.baseBlockProbability
                + request.quality.blockRisk * tuning.qualityBlockRiskWeight
                + laneFactor * tuning.laneFactorWeight
                + timingFactor * tuning.timingFactorWeight
                + skillFactor * tuning.skillFactorWeight
                + (request.context.lanePressure / 100.0) * tuning.lanePressureWeight
                - speedPenalty * tuning.speedPenaltyWeight,
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
                tuning.deflectionStrengthBase
                    + (request.execution.shotPower - tuning.deflectionStrengthPowerBaseline)
                        / tuning.deflectionStrengthPowerScale
                    + laneFactor * tuning.deflectionStrengthLaneWeight,
                tuning.minimumDeflectionStrength,
                tuning.maximumDeflectionStrength);
            bestProbability = probability;
        }
    }

    return best;
}
