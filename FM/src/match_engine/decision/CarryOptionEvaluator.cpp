#include"fm/match_engine/decision/CarryOptionEvaluator.h"

#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/geometry/TacticalZones.h"

#include<algorithm>
#include<cmath>
#include<limits>

namespace {
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

    double signedProgressX(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }

    bool isWide(PitchPoint point) {
        return point.y <= PitchGeometry::WidthMeters * 0.28
            || point.y >= PitchGeometry::WidthMeters * 0.72;
    }

    bool isCentral(PitchPoint point) {
        return std::abs(point.y - PitchGeometry::WidthMeters / 2.0)
            <= PitchGeometry::WidthMeters * 0.24;
    }

    double centralChannelShare(PitchPoint point) {
        const double distanceFromCenter =
            std::abs(point.y - PitchGeometry::WidthMeters / 2.0);
        return std::clamp(1.0 - (distanceFromCenter / (PitchGeometry::WidthMeters * 0.24)), 0.0, 1.0);
    }

    PitchPoint goalCenterFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    double goalDistance(PitchPoint point, AttackingDirection direction) {
        return PitchGeometry::distance(point, goalCenterFor(direction));
    }

    double minimumCentralCarryGoalDistance(CarryOptionKind kind, const CarryDecisionTuning& tuning) {
        switch (kind) {
        case CarryOptionKind::SafeCarry:
            return tuning.centralSafeCarryMinimumGoalDistance;
        case CarryOptionKind::ProgressiveCarry:
            return tuning.centralProgressiveCarryMinimumGoalDistance;
        case CarryOptionKind::Dribble:
        case CarryOptionKind::CutInside:
            return tuning.centralDribbleMinimumGoalDistance;
        }
        return tuning.centralProgressiveCarryMinimumGoalDistance;
    }

    PitchPoint contextAwareCarryTarget(
        PitchPoint position,
        AttackingDirection direction,
        double forward,
        double lateral,
        CarryOptionKind kind,
        const CarryDecisionTuning& tuning) {
        PitchPoint target = PitchGeometry::clampToPitch(PitchPoint{
            position.x + directionSign(direction) * forward,
            std::clamp(position.y + lateral, 2.0, PitchGeometry::WidthMeters - 2.0)
        });

        const double centralShare = centralChannelShare(target);
        const double minimumGoalDistance =
            (minimumCentralCarryGoalDistance(kind, tuning) * centralShare)
            + (tuning.halfSpaceCarryMinimumGoalDistance * (1.0 - centralShare));
        const double maxGoalLineProgress =
            PitchGeometry::LengthMeters - tuning.minimumCarryGoalLineDistance;
        if (goalDistance(target, direction) >= minimumGoalDistance
            && signedProgressX(target, direction) <= maxGoalLineProgress) {
            return target;
        }

        const double goalX = direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters
            : 0.0;
        target.x = goalX - directionSign(direction) * std::max(
            minimumGoalDistance,
            tuning.minimumCarryGoalLineDistance);
        return PitchGeometry::clampToPitch(target);
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

    bool isWingerRole(FormationSlotRole role) {
        return role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger
            || role == FormationSlotRole::LeftMidfielder
            || role == FormationSlotRole::RightMidfielder;
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

    double goalmouthCarryPressureRisk(
        PitchPoint target,
        AttackingDirection direction,
        const TeamSimState* opponentState,
        double skill,
        const CarryDecisionTuning& tuning) {
        const double distance = goalDistance(target, direction);
        if (distance > tuning.goalmouthPressureRiskDistance) {
            return 0.0;
        }

        const double centralShare = centralChannelShare(target);
        if (centralShare <= 0.05) {
            return 0.0;
        }

        double risk = 0.0;
        if (distance <= 4.0) {
            risk += tuning.sixYardPressureRisk;
        } else if (distance <= 8.0) {
            risk += tuning.closeBoxPressureRisk;
        } else {
            risk += tuning.boxPressureRisk;
        }

        if (opponentState != nullptr) {
            for (const PlayerSimState& opponent : opponentState->players) {
                const double opponentDistance = PitchGeometry::distance(target, opponent.position);
                if (opponentDistance <= 6.0) {
                    risk += tuning.nearbyCollapseRisk;
                } else if (opponentDistance <= 11.0) {
                    risk += tuning.nearbyCollapseRisk * 0.45;
                }
            }
        }

        if (distance <= 8.0) {
            risk += tuning.goalkeeperCloseDownRisk;
        }

        risk -= std::max(0.0, skill - 70.0) * tuning.eliteCarryRiskReduction;
        return std::clamp(risk * centralShare, 0.0, 100.0);
    }

    double zoneLimitRiskFor(
        FormationSlotRole role,
        PitchPoint from,
        PitchPoint target,
        AttackingDirection direction) {
        const CarryRoleDecisionProfile profile = carryRoleDecisionProfile(role);
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
        if (goalDistance(target, direction) <= 10.0 && isCentral(target)) {
            risk += (10.0 - goalDistance(target, direction)) * 8.0;
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
        } else if (kind == CarryOptionKind::Dribble
            || kind == CarryOptionKind::CutInside) {
            difficulty += 16.0;
        } else {
            difficulty -= 5.0;
        }
        return std::clamp(difficulty, 0.0, 100.0);
    }

    double roleKindMultiplier(
        const CarryRoleDecisionProfile& role,
        CarryOptionKind kind,
        PitchPoint target) {
        double multiplier = 1.0;
        if (kind == CarryOptionKind::SafeCarry) {
            multiplier *= role.safeBias;
        } else if (kind == CarryOptionKind::ProgressiveCarry) {
            multiplier *= role.progressiveBias;
        } else if (kind == CarryOptionKind::Dribble
            || kind == CarryOptionKind::CutInside) {
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
        const CarryTacticalDecisionProfile& tactics,
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
            return tuning.safeCarryBaseScore;
        case CarryOptionKind::ProgressiveCarry:
            return tuning.progressiveCarryBaseScore;
        case CarryOptionKind::Dribble:
        case CarryOptionKind::CutInside:
            return tuning.dribbleBaseScore;
        }
        return tuning.fallbackBaseScore;
    }

    BallCarrierActionType actionTypeFor(CarryOptionKind kind) {
        if (kind == CarryOptionKind::Dribble) {
            return BallCarrierActionType::Dribble;
        }
        if (kind == CarryOptionKind::CutInside) {
            return BallCarrierActionType::CutInside;
        }
        return BallCarrierActionType::Carry;
    }

    CarryOption makeOption(
        CarryOptionKind kind,
        const CarryOptionEvaluationContext& context,
        PitchPoint target,
        const PlayerAttributes& attributes,
        const CarryRoleDecisionProfile& role,
        const CarryTacticalDecisionProfile& tactics,
        const CarryDecisionTuning& tuning) {
        const double distance = PitchGeometry::distance(context.ballPosition, target);
        if (distance < tuning.minimumUsefulCarryDistance) {
            CarryOption option;
            option.kind = kind;
            option.actionType = actionTypeFor(kind);
            option.targetPoint = PitchGeometry::clampToPitch(target);
            return option;
        }

        const double skill = kind == CarryOptionKind::Dribble
                || kind == CarryOptionKind::CutInside
            ? dribbleSkill(attributes)
            : carrySkill(attributes);
        const double space = spaceScoreFor(target, context.opponentState);
        const double pressureRisk =
            std::clamp(
                pressureRiskFor(context.ballPosition, target, context.opponentState, context.carrierPressure)
                    + goalmouthCarryPressureRisk(
                        target,
                        context.attackingDirection,
                        context.opponentState,
                        skill,
                        tuning),
                0.0,
                100.0);
        const double progression =
            progressionScoreFor(kind, context.ballPosition, target, context.attackingDirection);
        const double zoneRisk =
            zoneLimitRiskFor(context.carrierRole, context.ballPosition, target, context.attackingDirection);
        const double riskTolerance = std::clamp(
            role.riskTolerance * tactics.riskTolerance,
            tuning.riskToleranceMinimum,
            tuning.riskToleranceMaximum);
        const double difficulty = controlDifficultyFor(kind, distance, pressureRisk, skill);

        double score = baselineFor(tuning, kind)
            + space * (kind == CarryOptionKind::SafeCarry
                ? tuning.safeSpaceContribution
                : tuning.riskSpaceContribution)
            + progression * (kind == CarryOptionKind::SafeCarry
                ? tuning.safeProgressionContribution
                : tuning.riskProgressionContribution)
            + skill * (kind == CarryOptionKind::Dribble
                    || kind == CarryOptionKind::CutInside
                ? tuning.dribbleSkillContribution
                : tuning.carrySkillContribution)
            - difficulty * tuning.difficultyPenalty
            - pressureRisk * tuning.pressureRiskPenalty / riskTolerance
            - zoneRisk * tuning.zoneLimitRiskPenalty / riskTolerance;

        score *= roleKindMultiplier(role, kind, target);
        score *= tacticalKindMultiplier(tactics, kind);

        if (kind == CarryOptionKind::SafeCarry
            && context.carrierPressure <= tuning.lowPressureSafeCarryThreshold) {
            score += tuning.lowPressureSafeCarryBonus;
        }
        if (kind == CarryOptionKind::ProgressiveCarry && space >= tuning.openSpaceProgressiveThreshold) {
            score += tuning.openSpaceProgressiveBonus;
        }
        if ((kind == CarryOptionKind::Dribble
                || kind == CarryOptionKind::CutInside)
            && context.carrierPressure >= tuning.highPressureDribbleThreshold) {
            score -= tuning.highPressureDribblePenalty;
        }
        if (isFullbackOrWideRole(context.carrierRole) && isWide(context.ballPosition) && isWide(target)) {
            score += kind == CarryOptionKind::SafeCarry
                ? tuning.wideSafeCarryBonus
                : tuning.wideProgressiveCarryBonus;
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
    const CarryRoleDecisionProfile role = carryRoleDecisionProfile(context.carrierRole);
    const CarryTacticalDecisionProfile tactics = carryTacticalDecisionProfile(context.tacticalSetup);
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
        contextAwareCarryTarget(
            context.ballPosition,
            context.attackingDirection,
            6.0,
            lateralTowardLane,
            CarryOptionKind::SafeCarry,
            tuning),
        attributes,
        role,
        tactics,
        tuning);
    pushIfUseful(
        output,
        safe,
        context.carrierRole == FormationSlotRole::Goalkeeper
            ? tuning.goalkeeperMinimumSafeCarryScore
            : tuning.minimumSafeCarryScore);

    if (context.carrierRole != FormationSlotRole::Goalkeeper) {
        const CarryOption progressive = makeOption(
            CarryOptionKind::ProgressiveCarry,
            context,
            contextAwareCarryTarget(
                context.ballPosition,
                context.attackingDirection,
                wideCarrier ? 15.0 : 13.0,
                lateralTowardLane,
                CarryOptionKind::ProgressiveCarry,
                tuning),
            attributes,
            role,
            tactics,
            tuning);
        pushIfUseful(output, progressive, tuning.minimumProgressiveCarryScore);

        const CarryOption dribble = makeOption(
            CarryOptionKind::Dribble,
            context,
            contextAwareCarryTarget(
                context.ballPosition,
                context.attackingDirection,
                wideCarrier ? 11.0 : 9.0,
                lateralDribble,
                CarryOptionKind::Dribble,
                tuning),
            attributes,
            role,
            tactics,
                tuning);
        pushIfUseful(output, dribble, tuning.minimumDribbleScore);

        if (wideCarrier
            && isWingerRole(context.carrierRole)
            && attackingProgress(context.ballPosition, context.attackingDirection) >= 0.66) {
            const double lateralInside = std::clamp(
                (PitchGeometry::WidthMeters / 2.0) - context.ballPosition.y,
                -14.0,
                14.0);
            const CarryOption cutInside = makeOption(
                CarryOptionKind::CutInside,
                context,
                contextAwareCarryTarget(
                    context.ballPosition,
                    context.attackingDirection,
                    7.0,
                    lateralInside,
                    CarryOptionKind::CutInside,
                    tuning),
                attributes,
                role,
                tactics,
                tuning);
            pushIfUseful(output, cutInside, tuning.minimumDribbleScore);
        }
    }

    std::sort(
        output.begin(),
        output.end(),
        [](const CarryOption& lhs, const CarryOption& rhs) {
            return lhs.score > rhs.score;
        });
    return output;
}
