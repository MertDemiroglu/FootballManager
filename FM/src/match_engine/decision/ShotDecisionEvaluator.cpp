#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

#include"fm/match_engine/ball/ShotQualityModel.h"
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
            profile.shotBias = 0.01;
            profile.longShotBias = 0.01;
            profile.riskTolerance = 0.45;
            break;
        case FormationSlotRole::CenterBack:
            profile.shotBias = 0.12;
            profile.longShotBias = 0.05;
            profile.riskTolerance = 0.55;
            break;
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            profile.shotBias = 0.32;
            profile.longShotBias = 0.22;
            profile.riskTolerance = 0.72;
            break;
        case FormationSlotRole::DefensiveMidfielder:
            profile.shotBias = 0.40;
            profile.longShotBias = 0.32;
            profile.riskTolerance = 0.74;
            break;
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
            profile.shotBias = 0.58;
            profile.longShotBias = 0.62;
            profile.riskTolerance = 0.92;
            break;
        case FormationSlotRole::AttackingMidfielder:
            profile.shotBias = 1.06;
            profile.longShotBias = 1.04;
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
        if (xg < 0.03) {
            return 2.0 + xg * 80.0;
        }
        if (xg < 0.08) {
            return 8.0 + (xg - 0.03) * 260.0;
        }
        if (xg < 0.18) {
            return 24.0 + (xg - 0.08) * 260.0;
        }
        if (xg < 0.30) {
            return 52.0 + (xg - 0.18) * 210.0;
        }
        return std::min(92.0, 78.0 + (xg - 0.30) * 130.0);
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

    const bool highValueChance = xg >= 0.18;
    if (xg < 0.03 && !highValueChance) {
        return output;
    }
    if (!isAttackingThird(zone) && xg < 0.18) {
        return output;
    }
    if (isDefensiveRole(context.carrierRole) && xg < 0.12) {
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

    double score = 3.0 * tuning.openPlayShotBaseline
        + xgDesire(xg) * 0.25
        + distanceScore * 0.06
        + angleScore * 0.04
        + shooter * 0.04
        - pressurePenalty * 0.32 / std::clamp(role.riskTolerance, 0.45, 1.35);

    score *= role.shotBias;
    if (distance > 24.0) {
        score *= role.longShotBias;
    }
    score *= tactics.shotBias;
    if (weakShot) {
        score *= tactics.weakShotBias;
    }
    if (isDefensiveRole(context.carrierRole) && distance > 18.0) {
        score -= 18.0;
    }
    const bool defensiveShooter = isDefensiveRole(context.carrierRole);
    if (xg >= 0.30) {
        score = std::min(score, defensiveShooter ? 16.0 : 20.0);
    } else if (xg >= 0.18) {
        score = defensiveShooter
            ? std::min(score, 15.0)
            : std::clamp(score, 18.0, 24.0);
    } else if (xg >= 0.08) {
        score = defensiveShooter
            ? std::min(score, 12.0)
            : std::clamp(score, 16.0, 22.0);
    } else {
        score = std::min(score, 22.0);
    }
    ShotOption option;
    option.kind = ShotOptionKind::OpenPlayShot;
    option.actionType = BallCarrierActionType::Shoot;
    option.targetPoint = goalCenterFor(context.attackingDirection);
    option.score = clampScore(score);
    option.estimatedXG = xg;
    option.angleScore = angleScore;
    option.distanceScore = distanceScore;
    option.pressurePenalty = pressurePenalty;
    option.shooterConfidence = shooter;

    if (option.score >= 5.0 || xg >= 0.18) {
        output.push_back(option);
    }
    return output;
}
