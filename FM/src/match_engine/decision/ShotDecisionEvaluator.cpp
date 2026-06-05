#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

#include"fm/match_engine/ball/ShotQualityModel.h"
#include"fm/match_engine/ball/ShotTargetModel.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/geometry/TacticalZones.h"

#include<algorithm>
#include<cmath>

namespace {
    constexpr double Pi = 3.14159265358979323846;

    double clampScore(double value) {
        return std::clamp(value, 0.0, 100.0);
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
        const double cosine = std::clamp(((ax * bx) + (ay * by)) / (lenA * lenB), -1.0, 1.0);
        return std::acos(cosine);
    }

    const MatchPlayerSnapshot* snapshotForPlayer(
        const MatchTeamSnapshot* snapshot,
        PlayerId playerId) {
        if (snapshot == nullptr) {
            return nullptr;
        }

        for (const MatchPlayerSnapshot& player : snapshot->players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }

        return nullptr;
    }

    PlayerAttributes attributesFor(
        const MatchTeamSnapshot* snapshot,
        const PlayerSimState* player) {
        if (player == nullptr) {
            return PlayerAttributes{};
        }

        const MatchPlayerSnapshot* playerSnapshot = snapshotForPlayer(snapshot, player->playerId);
        return playerSnapshot != nullptr ? playerSnapshot->attributes : PlayerAttributes{};
    }

    double shootingConfidence(const PlayerAttributes& attributes) {
        return std::clamp(
            attributes.technical.shooting * 0.34
                + attributes.technical.technique * 0.19
                + attributes.mental.composure * 0.18
                + attributes.mental.decisions * 0.15
                + attributes.mental.offTheBall * 0.08
                + attributes.physical.agility * 0.06,
            0.0,
            100.0);
    }

    double distanceScoreFor(double distance) {
        if (distance <= 8.0) {
            return 96.0;
        }
        if (distance <= 14.0) {
            return 84.0;
        }
        if (distance <= 20.0) {
            return 64.0;
        }
        if (distance <= 27.0) {
            return 38.0;
        }
        if (distance <= 34.0) {
            return 18.0;
        }
        return 6.0;
    }

    double angleScoreFor(PitchPoint point, AttackingDirection direction) {
        return clampScore((goalAngleRadians(point, direction) / Pi) * 100.0);
    }

    double xgDesire(double xg, const ShotDecisionTuning& tuning) {
        if (xg < tuning.xgVeryWeakThreshold) {
            return tuning.xgVeryWeakBase + xg * tuning.xgVeryWeakSlope;
        }
        if (xg < tuning.xgWeakThreshold) {
            return tuning.xgWeakBase + (xg - tuning.xgVeryWeakThreshold) * tuning.xgWeakSlope;
        }
        if (xg < tuning.xgLowThreshold) {
            return tuning.xgLowBase + (xg - tuning.xgWeakThreshold) * tuning.xgLowSlope;
        }
        if (xg < tuning.xgMediumThreshold) {
            return tuning.xgMediumBase + (xg - tuning.xgLowThreshold) * tuning.xgMediumSlope;
        }
        if (xg < tuning.xgGoodThreshold) {
            return tuning.xgGoodBase + (xg - tuning.xgMediumThreshold) * tuning.xgGoodSlope;
        }
        return std::min(
            tuning.xgDesireMaximum,
            tuning.xgGreatBase + (xg - tuning.xgGoodThreshold) * tuning.xgGreatSlope);
    }

    bool isDefensiveRole(FormationSlotRole role) {
        return role == FormationSlotRole::Goalkeeper
            || role == FormationSlotRole::CenterBack
            || role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack
            || role == FormationSlotRole::DefensiveMidfielder;
    }

}

std::vector<ShotOption> ShotDecisionEvaluator::evaluate(
    const ShotOptionEvaluationContext& context) const {
    std::vector<ShotOption> output;
    if (context.carrierState == nullptr) {
        return output;
    }

    const double distance =
        PitchGeometry::distance(context.ballPosition, goalCenterFor(context.attackingDirection));
    const double xg = ShotQualityModel::calculateOpenPlayXG(
        context.ballPosition,
        context.attackingDirection,
        context.carrierPressure);
    const TacticalZone zone =
        tacticalZoneForPoint(context.ballPosition, context.attackingDirection);
    const bool attackingThird = isAttackingThird(zone);

    const ShotDecisionTuning tuning;
    if (xg < tuning.minimumOpenPlayXG) {
        return output;
    }
    if (!attackingThird && xg < tuning.nonAttackingThirdMinimumXG) {
        return output;
    }
    const PlayerAttributes attributes = attributesFor(context.teamSnapshot, context.carrierState);
    const ShotRoleDecisionProfile role = shotRoleDecisionProfile(context.carrierRole);
    const ShotTacticalDecisionProfile tactics = shotTacticalDecisionProfile(context.tacticalSetup);
    const double shooter = shootingConfidence(attributes);
    const double distanceScore = distanceScoreFor(distance);
    const double angleScore = angleScoreFor(context.ballPosition, context.attackingDirection);
    const double pressurePenalty =
        std::clamp(context.carrierPressure * tuning.pressurePenaltyScale, 0.0, 100.0);
    const bool weakShot = xg < tuning.weakShotXG;
    const ShotTargetResult shotTarget = ShotTargetModel{}.chooseTarget(ShotTargetContext{
        context.ballPosition,
        context.attackingDirection,
        static_cast<double>(attributes.technical.shooting),
        static_cast<double>(attributes.technical.technique),
        static_cast<double>(attributes.mental.composure),
        context.carrierPressure
    });

    double score = xgDesire(xg, tuning) * tuning.openPlayShotBaseline
        + (distanceScore - tuning.distanceScoreBaseline) * tuning.distanceScoreContribution
        + (angleScore - tuning.angleScoreBaseline) * tuning.angleScoreContribution
        + (shooter - tuning.shooterConfidenceBaseline) * tuning.shooterConfidenceContribution
        - pressurePenalty * tuning.pressurePenaltyToScoreCost
            / std::clamp(role.riskTolerance, tuning.riskToleranceMinimum, tuning.riskToleranceMaximum);

    score *= std::clamp(
        1.0 + (role.shotBias - 1.0) * tuning.shotBiasBlend,
        tuning.shotBiasMinimum,
        tuning.shotBiasMaximum);
    if (distance > tuning.longShotDistance) {
        score *= std::clamp(
            1.0 + (role.longShotBias - 1.0) * tuning.longShotBiasBlend,
            tuning.longShotBiasMinimum,
            tuning.longShotBiasMaximum);
    }
    score *= std::clamp(
        1.0 + (tactics.shotBias - 1.0) * tuning.tacticalShotBiasBlend,
        tuning.tacticalShotBiasMinimum,
        tuning.tacticalShotBiasMaximum);
    if (weakShot) {
        score *= std::clamp(
            1.0 + (tactics.weakShotBias - 1.0) * tuning.weakShotBiasBlend,
            tuning.weakShotBiasMinimum,
            tuning.weakShotBiasMaximum);
    }
    if (isDefensiveRole(context.carrierRole) && xg < tuning.defensiveRoleLowXG) {
        score *= tuning.defensiveRoleWeakShotMultiplier;
    }

    ShotOption option;
    option.kind = ShotOptionKind::OpenPlayShot;
    option.actionType = BallCarrierActionType::Shoot;
    option.targetPoint = shotTarget.intendedTarget;
    option.score = std::clamp(score, 0.0, 100.0);
    option.estimatedXG = xg;
    option.angleScore = angleScore;
    option.distanceScore = distanceScore;
    option.pressurePenalty = pressurePenalty;
    option.shooterConfidence = shooter;

    if (option.score >= tuning.optionScoreMinimum || xg >= tuning.strongChanceAlwaysIncludeXG) {
        output.push_back(option);
    }
    return output;
}
