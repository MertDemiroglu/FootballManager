#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

#include"fm/match_engine/ball/ShotQualityModel.h"
#include"fm/match_engine/ball/ShotTargetModel.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/geometry/TacticalZones.h"

#include<algorithm>
#include<cmath>

namespace {
    constexpr double Pi = 3.14159265358979323846;

    struct ShotRoleProfile {
        double shotBias = 1.0;
        double longShotBias = 1.0;
        double riskTolerance = 1.0;
    };

    struct ShotTacticalProfile {
        double shotBias = 1.0;
        double weakShotBias = 1.0;
    };

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

    ShotRoleProfile roleProfile(FormationSlotRole role) {
        ShotRoleProfile profile;
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            profile.shotBias = 0.12;
            profile.longShotBias = 0.08;
            profile.riskTolerance = 0.45;
            break;
        case FormationSlotRole::CenterBack:
            profile.shotBias = 0.34;
            profile.longShotBias = 0.22;
            profile.riskTolerance = 0.58;
            break;
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            profile.shotBias = 0.58;
            profile.longShotBias = 0.46;
            profile.riskTolerance = 0.72;
            break;
        case FormationSlotRole::DefensiveMidfielder:
            profile.shotBias = 0.66;
            profile.longShotBias = 0.56;
            profile.riskTolerance = 0.74;
            break;
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
            profile.shotBias = 0.88;
            profile.longShotBias = 0.88;
            profile.riskTolerance = 0.92;
            break;
        case FormationSlotRole::AttackingMidfielder:
            profile.shotBias = 1.08;
            profile.longShotBias = 1.02;
            profile.riskTolerance = 1.10;
            break;
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            profile.shotBias = 0.98;
            profile.longShotBias = 0.92;
            profile.riskTolerance = 1.03;
            break;
        case FormationSlotRole::Striker:
            profile.shotBias = 1.18;
            profile.longShotBias = 1.02;
            profile.riskTolerance = 1.12;
            break;
        case FormationSlotRole::Unknown:
            break;
        }
        return profile;
    }

    ShotTacticalProfile tacticalProfile(const TacticalSetup& tactics) {
        ShotTacticalProfile profile;
        if (tactics.mentality == TeamMentality::Defensive) {
            profile.shotBias -= 0.16;
            profile.weakShotBias -= 0.22;
        } else if (tactics.mentality == TeamMentality::Attacking) {
            profile.shotBias += 0.14;
            profile.weakShotBias += 0.08;
        }

        if (tactics.tempo == TeamTempo::Low) {
            profile.shotBias -= 0.06;
            profile.weakShotBias -= 0.12;
        } else if (tactics.tempo == TeamTempo::High) {
            profile.shotBias += 0.07;
            profile.weakShotBias += 0.05;
        }

        if (tactics.passingDirectness == PassingDirectness::Direct) {
            profile.shotBias += 0.05;
        } else if (tactics.passingDirectness == PassingDirectness::Short) {
            profile.weakShotBias -= 0.08;
        }

        profile.shotBias = std::clamp(profile.shotBias, 0.55, 1.35);
        profile.weakShotBias = std::clamp(profile.weakShotBias, 0.50, 1.25);
        return profile;
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

    double xgDesire(double xg) {
        if (xg < 0.02) {
            return 4.0 + xg * 300.0;
        }
        if (xg < 0.05) {
            return 10.0 + (xg - 0.02) * 300.0;
        }
        if (xg < 0.08) {
            return 19.0 + (xg - 0.05) * 230.0;
        }
        if (xg < 0.18) {
            return 28.0 + (xg - 0.08) * 170.0;
        }
        if (xg < 0.30) {
            return 46.0 + (xg - 0.18) * 135.0;
        }
        return std::min(82.0, 62.0 + (xg - 0.30) * 70.0);
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

    if (xg < 0.012) {
        return output;
    }
    if (!attackingThird && xg < 0.06) {
        return output;
    }
    const PlayerAttributes attributes = attributesFor(context.teamSnapshot, context.carrierState);
    const ShotRoleProfile role = roleProfile(context.carrierRole);
    const ShotTacticalProfile tactics = tacticalProfile(context.tacticalSetup);
    const ShotDecisionTuning tuning;
    const double shooter = shootingConfidence(attributes);
    const double distanceScore = distanceScoreFor(distance);
    const double angleScore = angleScoreFor(context.ballPosition, context.attackingDirection);
    const double pressurePenalty =
        std::clamp(context.carrierPressure * tuning.pressurePenaltyScale, 0.0, 100.0);
    const bool weakShot = xg < 0.08;
    const ShotTargetResult shotTarget = ShotTargetModel{}.chooseTarget(ShotTargetContext{
        context.ballPosition,
        context.attackingDirection,
        static_cast<double>(attributes.technical.shooting),
        static_cast<double>(attributes.technical.technique),
        static_cast<double>(attributes.mental.composure),
        context.carrierPressure
    });

    double score = xgDesire(xg) * tuning.openPlayShotBaseline
        + (distanceScore - 50.0) * 0.05
        + (angleScore - 20.0) * 0.04
        + (shooter - 55.0) * 0.08
        - pressurePenalty * 0.16 / std::clamp(role.riskTolerance, 0.45, 1.35);

    score *= std::clamp(1.0 + (role.shotBias - 1.0) * 0.34, 0.58, 1.20);
    if (distance > 24.0) {
        score *= std::clamp(1.0 + (role.longShotBias - 1.0) * 0.40, 0.55, 1.10);
    }
    score *= std::clamp(1.0 + (tactics.shotBias - 1.0) * 0.45, 0.75, 1.16);
    if (weakShot) {
        score *= std::clamp(1.0 + (tactics.weakShotBias - 1.0) * 0.65, 0.62, 1.18);
    }
    if (isDefensiveRole(context.carrierRole) && xg < 0.12) {
        score *= 0.78;
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

    if (option.score >= 6.0 || xg >= 0.16) {
        output.push_back(option);
    }
    return output;
}
