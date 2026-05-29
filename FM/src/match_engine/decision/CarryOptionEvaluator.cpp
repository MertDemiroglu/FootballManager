#include"fm/match_engine/decision/CarryOptionEvaluator.h"

#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/geometry/TacticalZones.h"

#include<algorithm>
#include<cmath>
#include<limits>

namespace {
    struct CarryRoleProfile {
        double safeBias = 1.0;
        double progressiveBias = 1.0;
        double dribbleBias = 1.0;
        double wideBias = 1.0;
        double centralBias = 1.0;
        double riskTolerance = 1.0;
        double softProgressLimit = 0.70;
        double deepCarryPenalty = 0.0;
    };

    struct CarryTacticalProfile {
        double safeBias = 1.0;
        double progressiveBias = 1.0;
        double dribbleBias = 1.0;
        double riskTolerance = 1.0;
    };

    double clampScore(double value) {
        return std::clamp(value, 0.0, 100.0);
    }

    double directionSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    double attackingProgress(PitchPoint point, AttackingDirection direction) {
        const PitchPoint clamped = PitchGeometry::clampToPitch(point);
        if (direction == AttackingDirection::HomeToAway) {
            return clamped.x / PitchGeometry::LengthMeters;
        }
        return (PitchGeometry::LengthMeters - clamped.x) / PitchGeometry::LengthMeters;
    }

    double forwardMeters(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return (to.x - from.x) * directionSign(direction);
    }

    bool isWide(PitchPoint point) {
        return point.y <= PitchGeometry::WidthMeters * 0.28
            || point.y >= PitchGeometry::WidthMeters * 0.72;
    }

    bool isCentral(PitchPoint point) {
        return std::abs(point.y - PitchGeometry::WidthMeters / 2.0)
            <= PitchGeometry::WidthMeters * 0.24;
    }

    PitchPoint carryTarget(
        PitchPoint position,
        AttackingDirection direction,
        double forward,
        double lateral) {
        return PitchGeometry::clampToPitch(PitchPoint{
            position.x + directionSign(direction) * forward,
            std::clamp(position.y + lateral, 2.0, PitchGeometry::WidthMeters - 2.0)
        });
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

    double carrySkill(const PlayerAttributes& attributes) {
        return std::clamp(
            attributes.technical.dribbling * 0.28
                + attributes.technical.technique * 0.19
                + attributes.mental.decisions * 0.17
                + attributes.mental.composure * 0.12
                + attributes.physical.pace * 0.12
                + attributes.physical.acceleration * 0.12,
            0.0,
            100.0);
    }

    double dribbleSkill(const PlayerAttributes& attributes) {
        return std::clamp(
            attributes.technical.dribbling * 0.32
                + attributes.technical.technique * 0.18
                + attributes.physical.acceleration * 0.17
                + attributes.physical.agility * 0.13
                + attributes.mental.composure * 0.11
                + attributes.mental.decisions * 0.09,
            0.0,
            100.0);
    }

    bool isFullbackOrWideRole(FormationSlotRole role) {
        return role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack
            || role == FormationSlotRole::LeftMidfielder
            || role == FormationSlotRole::RightMidfielder
            || role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger;
    }

    CarryRoleProfile roleProfile(FormationSlotRole role) {
        CarryRoleProfile profile;
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            profile.safeBias = 0.30;
            profile.progressiveBias = 0.05;
            profile.dribbleBias = 0.02;
            profile.riskTolerance = 0.42;
            profile.softProgressLimit = 0.22;
            break;
        case FormationSlotRole::CenterBack:
            profile.safeBias = 1.18;
            profile.progressiveBias = 0.55;
            profile.dribbleBias = 0.10;
            profile.riskTolerance = 0.62;
            profile.softProgressLimit = 0.46;
            break;
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
            profile.safeBias = 1.08;
            profile.progressiveBias = 0.92;
            profile.dribbleBias = 0.58;
            profile.wideBias = 1.25;
            profile.riskTolerance = 0.82;
            profile.softProgressLimit = 0.66;
            break;
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            profile.safeBias = 1.02;
            profile.progressiveBias = 1.08;
            profile.dribbleBias = 0.76;
            profile.wideBias = 1.30;
            profile.riskTolerance = 0.92;
            profile.softProgressLimit = 0.74;
            break;
        case FormationSlotRole::DefensiveMidfielder:
            profile.safeBias = 1.14;
            profile.progressiveBias = 0.88;
            profile.dribbleBias = 0.38;
            profile.riskTolerance = 0.78;
            profile.softProgressLimit = 0.58;
            break;
        case FormationSlotRole::CentralMidfielder:
            profile.safeBias = 1.02;
            profile.progressiveBias = 1.04;
            profile.dribbleBias = 0.72;
            profile.riskTolerance = 0.96;
            profile.softProgressLimit = 0.70;
            break;
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
            profile.safeBias = 0.98;
            profile.progressiveBias = 1.08;
            profile.dribbleBias = 0.86;
            profile.wideBias = 1.16;
            profile.riskTolerance = 1.00;
            profile.softProgressLimit = 0.76;
            break;
        case FormationSlotRole::AttackingMidfielder:
            profile.safeBias = 0.86;
            profile.progressiveBias = 1.22;
            profile.dribbleBias = 1.16;
            profile.centralBias = 1.12;
            profile.riskTolerance = 1.12;
            profile.softProgressLimit = 0.88;
            break;
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            profile.safeBias = 0.84;
            profile.progressiveBias = 1.24;
            profile.dribbleBias = 1.28;
            profile.wideBias = 1.34;
            profile.riskTolerance = 1.10;
            profile.softProgressLimit = 0.92;
            break;
        case FormationSlotRole::Striker:
            profile.safeBias = 0.70;
            profile.progressiveBias = 0.82;
            profile.dribbleBias = 1.02;
            profile.riskTolerance = 0.94;
            profile.softProgressLimit = 0.94;
            profile.deepCarryPenalty = 14.0;
            break;
        case FormationSlotRole::Unknown:
            break;
        }
        return profile;
    }

    CarryTacticalProfile tacticalProfile(const TacticalSetup& tactics) {
        CarryTacticalProfile profile;
        if (tactics.mentality == TeamMentality::Defensive) {
            profile.safeBias += 0.18;
            profile.progressiveBias -= 0.14;
            profile.dribbleBias -= 0.16;
            profile.riskTolerance -= 0.12;
        } else if (tactics.mentality == TeamMentality::Attacking) {
            profile.safeBias -= 0.06;
            profile.progressiveBias += 0.18;
            profile.dribbleBias += 0.14;
            profile.riskTolerance += 0.14;
        }

        if (tactics.tempo == TeamTempo::Low) {
            profile.safeBias += 0.16;
            profile.progressiveBias -= 0.10;
            profile.dribbleBias -= 0.08;
            profile.riskTolerance -= 0.05;
        } else if (tactics.tempo == TeamTempo::High) {
            profile.safeBias -= 0.06;
            profile.progressiveBias += 0.16;
            profile.dribbleBias += 0.11;
            profile.riskTolerance += 0.08;
        }

        if (tactics.passingDirectness == PassingDirectness::Short) {
            profile.safeBias += 0.12;
            profile.progressiveBias -= 0.10;
        } else if (tactics.passingDirectness == PassingDirectness::Direct) {
            profile.safeBias -= 0.08;
            profile.progressiveBias += 0.16;
            profile.dribbleBias += 0.06;
        }

        profile.safeBias = std::clamp(profile.safeBias, 0.65, 1.40);
        profile.progressiveBias = std::clamp(profile.progressiveBias, 0.55, 1.45);
        profile.dribbleBias = std::clamp(profile.dribbleBias, 0.50, 1.45);
        profile.riskTolerance = std::clamp(profile.riskTolerance, 0.65, 1.35);
        return profile;
    }

    double nearestOpponentDistance(
        PitchPoint position,
        const TeamSimState* opponentState) {
        if (opponentState == nullptr || opponentState->players.empty()) {
            return 24.0;
        }

        double nearest = std::numeric_limits<double>::max();
        for (const PlayerSimState& opponent : opponentState->players) {
            nearest = std::min(nearest, PitchGeometry::distance(position, opponent.position));
        }
        return nearest;
    }

    double spaceScoreFor(PitchPoint target, const TeamSimState* opponentState) {
        const double nearest = nearestOpponentDistance(target, opponentState);
        if (nearest <= 2.5) {
            return 12.0;
        }
        if (nearest <= 5.0) {
            return 34.0;
        }
        if (nearest <= 8.0) {
            return 58.0;
        }
        if (nearest <= 13.0) {
            return 78.0;
        }
        return 92.0;
    }

    double pressureRiskFor(
        PitchPoint from,
        PitchPoint target,
        const TeamSimState* opponentState,
        double carrierPressure) {
        const double nearestTarget = nearestOpponentDistance(target, opponentState);
        double risk = carrierPressure * 0.62;
        if (nearestTarget <= 2.5) {
            risk += 38.0;
        } else if (nearestTarget <= 5.0) {
            risk += 24.0;
        } else if (nearestTarget <= 8.0) {
            risk += 12.0;
        } else if (nearestTarget <= 12.0) {
            risk += 5.0;
        }

        if (opponentState != nullptr) {
            const PitchPoint midpoint{
                (from.x + target.x) / 2.0,
                (from.y + target.y) / 2.0
            };
            for (const PlayerSimState& opponent : opponentState->players) {
                if (PitchGeometry::distance(midpoint, opponent.position) <= 4.5) {
                    risk += 6.0;
                }
            }
        }
        return std::clamp(risk, 0.0, 100.0);
    }

    double zoneLimitRiskFor(
        FormationSlotRole role,
        PitchPoint from,
        PitchPoint target,
        AttackingDirection direction) {
        const CarryRoleProfile profile = roleProfile(role);
        const double targetProgress = attackingProgress(target, direction);
        const double currentProgress = attackingProgress(from, direction);
        double risk = 0.0;
        if (targetProgress > profile.softProgressLimit) {
            risk += (targetProgress - profile.softProgressLimit) * 120.0;
        }
        if (role == FormationSlotRole::CenterBack
            && isAttackingThird(tacticalZoneForPoint(target, direction))) {
            risk += 18.0;
        }
        if (role == FormationSlotRole::Goalkeeper && targetProgress > 0.26) {
            risk += 40.0;
        }
        if (role == FormationSlotRole::Striker && currentProgress < 0.42) {
            risk += profile.deepCarryPenalty;
        }
        if (targetProgress > 0.78) {
            double deepFinalThirdRisk = (targetProgress - 0.78) * 260.0;
            if (role == FormationSlotRole::AttackingMidfielder
                || role == FormationSlotRole::LeftWinger
                || role == FormationSlotRole::RightWinger
                || role == FormationSlotRole::Striker) {
                deepFinalThirdRisk *= 0.80;
            }
            risk += deepFinalThirdRisk;
        }
        return std::clamp(risk, 0.0, 100.0);
    }

    double progressionScoreFor(
        CarryOptionKind kind,
        PitchPoint from,
        PitchPoint target,
        AttackingDirection direction) {
        const double forward = std::max(0.0, forwardMeters(from, target, direction));
        double score = forward * (kind == CarryOptionKind::SafeCarry ? 1.0 : 1.75);
        const TacticalZone fromZone = tacticalZoneForPoint(from, direction);
        const TacticalZone targetZone = tacticalZoneForPoint(target, direction);
        if (!isAttackingThird(fromZone) && isAttackingThird(targetZone)) {
            score += 9.0;
        } else if (isDefensiveThird(fromZone) && isMiddleThird(targetZone)) {
            score += 7.0;
        }
        if (kind == CarryOptionKind::Dribble) {
            score += 6.0;
        }
        return clampScore(score);
    }

    double controlDifficultyFor(
        CarryOptionKind kind,
        double distance,
        double pressureRisk,
        double skill) {
        double difficulty = distance * 1.05 + pressureRisk * 0.42 - skill * 0.22;
        if (kind == CarryOptionKind::ProgressiveCarry) {
            difficulty += 7.0;
        } else if (kind == CarryOptionKind::Dribble) {
            difficulty += 16.0;
        } else {
            difficulty -= 5.0;
        }
        return std::clamp(difficulty, 0.0, 100.0);
    }

    double roleKindMultiplier(
        const CarryRoleProfile& role,
        CarryOptionKind kind,
        PitchPoint target) {
        double multiplier = 1.0;
        if (kind == CarryOptionKind::SafeCarry) {
            multiplier *= role.safeBias;
        } else if (kind == CarryOptionKind::ProgressiveCarry) {
            multiplier *= role.progressiveBias;
        } else if (kind == CarryOptionKind::Dribble) {
            multiplier *= role.dribbleBias;
        }

        if (isWide(target)) {
            multiplier *= role.wideBias;
        }
        if (isCentral(target)) {
            multiplier *= role.centralBias;
        }
        return std::clamp(multiplier, 0.20, 1.55);
    }

    double tacticalKindMultiplier(
        const CarryTacticalProfile& tactics,
        CarryOptionKind kind) {
        if (kind == CarryOptionKind::SafeCarry) {
            return tactics.safeBias;
        }
        if (kind == CarryOptionKind::ProgressiveCarry) {
            return tactics.progressiveBias;
        }
        return tactics.dribbleBias;
    }

    double baselineFor(const CarryDecisionTuning& tuning, CarryOptionKind kind) {
        switch (kind) {
        case CarryOptionKind::SafeCarry:
            return 35.0 * tuning.safeCarryBaseline;
        case CarryOptionKind::ProgressiveCarry:
            return 34.0 * tuning.progressiveCarryBaseline;
        case CarryOptionKind::Dribble:
            return 27.0 * tuning.dribbleBaseline;
        }
        return 20.0;
    }

    BallCarrierActionType actionTypeFor(CarryOptionKind kind) {
        return kind == CarryOptionKind::Dribble
            ? BallCarrierActionType::Dribble
            : BallCarrierActionType::Carry;
    }

    CarryOption makeOption(
        CarryOptionKind kind,
        const CarryOptionEvaluationContext& context,
        PitchPoint target,
        const PlayerAttributes& attributes,
        const CarryRoleProfile& role,
        const CarryTacticalProfile& tactics,
        const CarryDecisionTuning& tuning) {
        const double distance = PitchGeometry::distance(context.ballPosition, target);
        const double skill = kind == CarryOptionKind::Dribble
            ? dribbleSkill(attributes)
            : carrySkill(attributes);
        const double space = spaceScoreFor(target, context.opponentState);
        const double pressureRisk =
            pressureRiskFor(context.ballPosition, target, context.opponentState, context.carrierPressure);
        const double progression =
            progressionScoreFor(kind, context.ballPosition, target, context.attackingDirection);
        const double zoneRisk =
            zoneLimitRiskFor(context.carrierRole, context.ballPosition, target, context.attackingDirection);
        const double riskTolerance = std::clamp(role.riskTolerance * tactics.riskTolerance, 0.45, 1.55);
        const double difficulty = controlDifficultyFor(kind, distance, pressureRisk, skill);

        double score = baselineFor(tuning, kind)
            + space * (kind == CarryOptionKind::SafeCarry ? 0.27 : 0.20)
            + progression * (kind == CarryOptionKind::SafeCarry ? 0.12 : 0.34)
            + skill * (kind == CarryOptionKind::Dribble ? 0.25 : 0.13)
            - difficulty * 0.15
            - pressureRisk * 0.18 / riskTolerance
            - zoneRisk * 0.34 / riskTolerance;

        score *= roleKindMultiplier(role, kind, target);
        score *= tacticalKindMultiplier(tactics, kind);

        if (kind == CarryOptionKind::SafeCarry && context.carrierPressure <= 8.0) {
            score += 5.0;
        }
        if (kind == CarryOptionKind::ProgressiveCarry && space >= 72.0) {
            score += 4.0;
        }
        if (kind == CarryOptionKind::Dribble && context.carrierPressure >= 35.0) {
            score -= 8.0;
        }
        if (isFullbackOrWideRole(context.carrierRole) && isWide(context.ballPosition) && isWide(target)) {
            score += kind == CarryOptionKind::SafeCarry ? 2.0 : 6.0;
        }

        CarryOption option;
        option.kind = kind;
        option.actionType = actionTypeFor(kind);
        option.targetPoint = PitchGeometry::clampToPitch(target);
        option.score = clampScore(score);
        option.spaceScore = space;
        option.progressionScore = progression;
        option.controlDifficulty = difficulty;
        option.pressureRisk = pressureRisk;
        option.zoneLimitRisk = zoneRisk;
        return option;
    }

    void pushIfUseful(std::vector<CarryOption>& output, const CarryOption& option, double minimumScore) {
        if (option.score >= minimumScore) {
            output.push_back(option);
        }
    }
}

std::vector<CarryOption> CarryOptionEvaluator::evaluate(
    const CarryOptionEvaluationContext& context) const {
    std::vector<CarryOption> output;
    if (context.carrierState == nullptr || context.opponentState == nullptr) {
        return output;
    }

    const PlayerAttributes attributes = attributesFor(context.teamSnapshot, context.carrierState);
    const CarryRoleProfile role = roleProfile(context.carrierRole);
    const CarryTacticalProfile tactics = tacticalProfile(context.tacticalSetup);
    const CarryDecisionTuning tuning;
    const bool wideCarrier = isWide(context.ballPosition);

    const double lateralTowardLane = wideCarrier
        ? (context.ballPosition.y < PitchGeometry::WidthMeters / 2.0 ? -1.5 : 1.5)
        : 0.0;
    const double lateralDribble = wideCarrier
        ? (context.ballPosition.y < PitchGeometry::WidthMeters / 2.0 ? 2.0 : -2.0)
        : 3.0;

    const CarryOption safe = makeOption(
        CarryOptionKind::SafeCarry,
        context,
        carryTarget(context.ballPosition, context.attackingDirection, 6.0, lateralTowardLane),
        attributes,
        role,
        tactics,
        tuning);
    pushIfUseful(output, safe, context.carrierRole == FormationSlotRole::Goalkeeper ? 14.0 : 18.0);

    if (context.carrierRole != FormationSlotRole::Goalkeeper) {
        const CarryOption progressive = makeOption(
            CarryOptionKind::ProgressiveCarry,
            context,
            carryTarget(context.ballPosition, context.attackingDirection, wideCarrier ? 15.0 : 13.0, lateralTowardLane),
            attributes,
            role,
            tactics,
            tuning);
        pushIfUseful(output, progressive, 12.0);

        const CarryOption dribble = makeOption(
            CarryOptionKind::Dribble,
            context,
            carryTarget(context.ballPosition, context.attackingDirection, wideCarrier ? 11.0 : 9.0, lateralDribble),
            attributes,
            role,
            tactics,
            tuning);
        pushIfUseful(output, dribble, 8.0);
    }

    std::sort(
        output.begin(),
        output.end(),
        [](const CarryOption& lhs, const CarryOption& rhs) {
            return lhs.score > rhs.score;
        });
    return output;
}
