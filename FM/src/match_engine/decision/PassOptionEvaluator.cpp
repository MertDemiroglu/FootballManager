#include"fm/match_engine/decision/PassOptionEvaluator.h"

#include"fm/match_engine/decision/DecisionTuningProfile.h"

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

    double forwardMeters(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return (to.x - from.x) * directionSign(direction);
    }

    bool isAhead(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return forwardMeters(from, to, direction) > 1.0;
    }

    bool isBehind(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return forwardMeters(from, to, direction) < -1.0;
    }

    bool isWide(PitchPoint point) {
        return point.y <= PitchGeometry::WidthMeters * 0.22
            || point.y >= PitchGeometry::WidthMeters * 0.78;
    }

    bool isOppositeWideSide(PitchPoint from, PitchPoint to) {
        const bool fromLeft = from.y < PitchGeometry::WidthMeters / 2.0;
        const bool toLeft = to.y < PitchGeometry::WidthMeters / 2.0;
        return fromLeft != toLeft && std::abs(from.y - to.y) >= PitchGeometry::WidthMeters * 0.42;
    }

    bool isFinalThird(PitchPoint point, AttackingDirection direction) {
        if (direction == AttackingDirection::HomeToAway) {
            return point.x >= PitchGeometry::LengthMeters * 0.66;
        }

        return point.x <= PitchGeometry::LengthMeters * 0.34;
    }

    bool isAttackingHalf(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x >= PitchGeometry::LengthMeters * 0.50
            : point.x <= PitchGeometry::LengthMeters * 0.50;
    }

    bool isInOpponentBox(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::isInsideAwayPenaltyArea(point)
            : PitchGeometry::isInsideHomePenaltyArea(point);
    }

    PitchPoint lowCrossTarget(AttackingDirection direction) {
        const double x = direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters - 11.0
            : 11.0;

        return PitchPoint{ x, PitchGeometry::WidthMeters / 2.0 };
    }

    PitchPoint cutbackTarget(PitchPoint ballPosition, AttackingDirection direction) {
        const double x = ballPosition.x - directionSign(direction) * 8.0;
        return PitchGeometry::clampToPitch(PitchPoint{
            x,
            PitchGeometry::WidthMeters / 2.0
        });
    }

    PitchPoint throughBallTarget(PitchPoint receiverPosition, AttackingDirection direction) {
        return PitchGeometry::clampToPitch(PitchPoint{
            receiverPosition.x + directionSign(direction) * 7.5,
            receiverPosition.y
        });
    }

    double distancePointToSegment(PitchPoint point, PitchPoint a, PitchPoint b) {
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double lengthSquared = dx * dx + dy * dy;
        if (lengthSquared <= 0.001) {
            return PitchGeometry::distance(point, a);
        }

        const double t = std::clamp(
            (((point.x - a.x) * dx) + ((point.y - a.y) * dy)) / lengthSquared,
            0.0,
            1.0);
        return PitchGeometry::distance(point, PitchPoint{ a.x + dx * t, a.y + dy * t });
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

    FormationSlotRole roleForPlayer(const MatchTeamSnapshot* snapshot, PlayerId playerId) {
        if (snapshot == nullptr) {
            return FormationSlotRole::Unknown;
        }

        for (const TeamSheetSlotAssignment& assignment : snapshot->teamSheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }

        return FormationSlotRole::Unknown;
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

    double passingSkill(const PlayerAttributes& attributes) {
        return std::clamp(
            attributes.technical.passing * 0.32
                + attributes.mental.vision * 0.24
                + attributes.mental.decisions * 0.19
                + attributes.technical.technique * 0.15
                + attributes.mental.composure * 0.10,
            0.0,
            100.0);
    }

    double executionSkill(const PlayerAttributes& attributes, PassOptionKind kind) {
        const double crossing = kind == PassOptionKind::Cross || kind == PassOptionKind::Cutback
            ? attributes.technical.crossing * 0.22
            : attributes.technical.passing * 0.22;
        return std::clamp(
            crossing
                + attributes.technical.passing * 0.20
                + attributes.technical.technique * 0.20
                + attributes.mental.vision * 0.14
                + attributes.mental.decisions * 0.13
                + attributes.mental.composure * 0.11,
            0.0,
            100.0);
    }

    bool isDefenderRole(FormationSlotRole role) {
        return role == FormationSlotRole::CenterBack
            || role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack;
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

    double receiverPressure(
        PitchPoint receiverPosition,
        const TeamSimState* opponentState) {
        if (opponentState == nullptr || opponentState->players.empty()) {
            return 0.0;
        }

        double nearest = std::numeric_limits<double>::max();
        int nearby = 0;
        for (const PlayerSimState& opponent : opponentState->players) {
            const double distance = PitchGeometry::distance(receiverPosition, opponent.position);
            nearest = std::min(nearest, distance);
            if (distance <= 7.0) {
                ++nearby;
            }
        }

        double pressure = 0.0;
        if (nearest <= 2.5) {
            pressure += 42.0;
        } else if (nearest <= 5.0) {
            pressure += 28.0;
        } else if (nearest <= 8.0) {
            pressure += 15.0;
        } else if (nearest <= 12.0) {
            pressure += 7.0;
        }

        pressure += std::max(0, nearby - 1) * 8.0;
        return std::min(65.0, pressure);
    }

    double laneRisk(
        PitchPoint from,
        PitchPoint to,
        PassOptionKind kind,
        const TeamSimState* opponentState) {
        if (opponentState == nullptr) {
            return 0.0;
        }

        double risk = 0.0;
        for (const PlayerSimState& opponent : opponentState->players) {
            const double pathDistance = distancePointToSegment(opponent.position, from, to);
            if (pathDistance <= 2.5) {
                risk += 20.0;
            } else if (pathDistance <= 4.5) {
                risk += 11.0;
            } else if (pathDistance <= 7.0) {
                risk += 4.5;
            }
        }

        const double distance = PitchGeometry::distance(from, to);
        risk += std::max(0.0, distance - 22.0) * 0.12;
        if (kind == PassOptionKind::ThroughBall || kind == PassOptionKind::Cross) {
            risk *= 1.12;
        } else if (kind == PassOptionKind::BackPass || kind == PassOptionKind::SafePass) {
            risk *= 0.82;
        }

        return std::min(72.0, risk);
    }

    double executionDifficulty(
        double distance,
        double lane,
        double pressure,
        double carrierPressure,
        PassOptionKind kind,
        double skill) {
        double difficulty = distance * 0.72
            + lane * 0.46
            + pressure * 0.34
            + carrierPressure * 0.22
            - skill * 0.18;
        if (kind == PassOptionKind::ThroughBall) {
            difficulty += 11.0;
        } else if (kind == PassOptionKind::SwitchPlay) {
            difficulty += 9.0;
        } else if (kind == PassOptionKind::Cross) {
            difficulty += 8.0;
        } else if (kind == PassOptionKind::Cutback) {
            difficulty += 4.0;
        } else if (kind == PassOptionKind::BackPass) {
            difficulty -= 6.0;
        }
        return std::clamp(difficulty, 0.0, 100.0);
    }

    double receiverRoleProgressionBonus(FormationSlotRole role, PassOptionKind kind) {
        if (kind == PassOptionKind::SafePass || kind == PassOptionKind::BackPass) {
            return role == FormationSlotRole::DefensiveMidfielder
                || role == FormationSlotRole::CentralMidfielder ? 3.0 : 0.0;
        }
        if (role == FormationSlotRole::AttackingMidfielder
            || role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger
            || role == FormationSlotRole::Striker) {
            return kind == PassOptionKind::ThroughBall ? 8.0 : 5.0;
        }
        if (isFullbackOrWideRole(role) && kind == PassOptionKind::SwitchPlay) {
            return 6.0;
        }
        return 0.0;
    }

    double roleKindMultiplier(const RoleRiskProfile& role, PassOptionKind kind) {
        switch (kind) {
        case PassOptionKind::SafePass:
            return role.safeOptionBias;
        case PassOptionKind::BackPass:
            return role.safeOptionBias * 1.05;
        case PassOptionKind::ProgressivePass:
            return role.progressiveOptionBias;
        case PassOptionKind::SwitchPlay:
            return role.directOptionBias * role.wideOptionBias;
        case PassOptionKind::ThroughBall:
            return role.directOptionBias * role.throughBallBias;
        case PassOptionKind::Cross:
        case PassOptionKind::Cutback:
            return role.crossOptionBias * role.wideOptionBias;
        }

        return 1.0;
    }

    double tacticalKindMultiplier(const TacticalBiasProfile& tactics, PassOptionKind kind) {
        switch (kind) {
        case PassOptionKind::SafePass:
            return tactics.safeCirculationBias * tactics.possessionBias;
        case PassOptionKind::BackPass:
            return tactics.safeCirculationBias * (0.95 + (tactics.possessionBias - 1.0) * 0.5);
        case PassOptionKind::ProgressivePass:
            return tactics.verticalityBias * tactics.directPassingBias;
        case PassOptionKind::SwitchPlay:
            return tactics.directPassingBias * tactics.widthBias;
        case PassOptionKind::ThroughBall:
            return tactics.verticalityBias * tactics.directPassingBias;
        case PassOptionKind::Cross:
        case PassOptionKind::Cutback:
            return tactics.widthBias * tactics.verticalityBias;
        }

        return 1.0;
    }

    PassDecisionTuning passTuning() {
        return PassDecisionTuning{};
    }

    double baselineFor(const PassDecisionTuning& tuning, PassOptionKind kind) {
        switch (kind) {
        case PassOptionKind::SafePass:
            return 33.0 * tuning.safePassBaseline;
        case PassOptionKind::BackPass:
            return 32.0 * tuning.backPassBaseline;
        case PassOptionKind::ProgressivePass:
            return 28.0 * tuning.progressivePassBaseline;
        case PassOptionKind::SwitchPlay:
            return 25.0 * tuning.switchPlayBaseline;
        case PassOptionKind::ThroughBall:
            return 22.0 * tuning.throughBallBaseline;
        case PassOptionKind::Cross:
            return 24.0 * tuning.crossBaseline;
        case PassOptionKind::Cutback:
            return 24.0 * tuning.cutbackBaseline;
        }

        return 24.0;
    }

    BallCarrierActionType actionTypeFor(PassOptionKind kind) {
        switch (kind) {
        case PassOptionKind::BackPass:
            return BallCarrierActionType::BackPass;
        case PassOptionKind::SwitchPlay:
            return BallCarrierActionType::SwitchPlay;
        case PassOptionKind::ThroughBall:
            return BallCarrierActionType::ThroughBall;
        case PassOptionKind::Cross:
            return BallCarrierActionType::LowCross;
        case PassOptionKind::Cutback:
            return BallCarrierActionType::Cutback;
        case PassOptionKind::SafePass:
        case PassOptionKind::ProgressivePass:
            return BallCarrierActionType::ShortPass;
        }

        return BallCarrierActionType::ShortPass;
    }

    PassOption makeOption(
        PassOptionKind kind,
        const PassOptionEvaluationContext& context,
        const PlayerSimState& receiver,
        PitchPoint targetPoint,
        FormationSlotRole receiverRole,
        const PlayerAttributes& passerAttributes,
        const RoleRiskProfile& role,
        const TacticalBiasProfile& tactics,
        const PassDecisionTuning& tuning) {
        const double distance = PitchGeometry::distance(context.ballPosition, targetPoint);
        const double lane = laneRisk(context.ballPosition, targetPoint, kind, context.opponentState);
        const double pressure = receiverPressure(receiver.position, context.opponentState);
        const double skill = executionSkill(passerAttributes, kind);
        const double difficulty =
            executionDifficulty(distance, lane, pressure, context.carrierPressure, kind, skill);
        const double riskTolerance = std::clamp(role.riskTolerance * tactics.riskTolerance, 0.55, 1.55);
        const double safety = clampScore(
            95.0
                - lane * tuning.laneRiskPenaltyScale * 0.78 / riskTolerance
                - pressure * tuning.receiverPressurePenaltyScale * 0.48 / riskTolerance
                - difficulty * 0.50
                - context.carrierPressure * 0.18);

        const double forward = std::max(0.0, forwardMeters(
            context.ballPosition,
            receiver.position,
            context.attackingDirection));
        double progression = std::min(38.0, forward * 1.35)
            + (isFinalThird(receiver.position, context.attackingDirection) ? 8.0 : 0.0)
            + receiverRoleProgressionBonus(receiverRole, kind);
        if (kind == PassOptionKind::SwitchPlay) {
            progression += 7.0;
        } else if (kind == PassOptionKind::ThroughBall) {
            progression += 13.0;
        } else if (kind == PassOptionKind::Cross || kind == PassOptionKind::Cutback) {
            progression += 10.0;
        } else if (kind == PassOptionKind::BackPass) {
            progression *= 0.18;
        }
        progression = clampScore(progression);

        double score = baselineFor(tuning, kind)
            + safety * (kind == PassOptionKind::SafePass || kind == PassOptionKind::BackPass ? 0.34 : 0.20)
            + progression * (kind == PassOptionKind::SafePass || kind == PassOptionKind::BackPass ? 0.10 : 0.33)
            + passingSkill(passerAttributes) * 0.10
            - difficulty * 0.12;

        if (kind == PassOptionKind::SafePass && distance <= 15.0) {
            score += 5.0;
        }
        if (isFinalThird(context.ballPosition, context.attackingDirection)
            && (kind == PassOptionKind::SafePass || kind == PassOptionKind::ProgressivePass)) {
            score += 9.0;
        }
        if (kind == PassOptionKind::BackPass && isDefenderRole(context.carrierRole)) {
            score += 3.0;
        }

        const double roleMultiplier =
            std::clamp(1.0 + (roleKindMultiplier(role, kind) - 1.0) * 0.62, 0.58, 1.34);
        const double tacticalMultiplier =
            std::clamp(1.0 + (tacticalKindMultiplier(tactics, kind) - 1.0) * 0.62, 0.58, 1.34);
        score *= roleMultiplier;
        score *= tacticalMultiplier;
        if (kind == PassOptionKind::ProgressivePass) {
            score -= 6.0;
        } else if (kind == PassOptionKind::SwitchPlay) {
            score -= 8.0;
        } else if (kind == PassOptionKind::ThroughBall) {
            score -= 18.0;
        } else if (kind == PassOptionKind::Cross || kind == PassOptionKind::Cutback) {
            score -= 20.0;
        }

        PassOption option;
        option.kind = kind;
        option.actionType = actionTypeFor(kind);
        option.targetPlayerId = receiver.playerId;
        option.targetPoint = PitchGeometry::clampToPitch(targetPoint);
        option.score = clampScore(score);
        option.safetyScore = safety;
        option.progressionScore = progression;
        option.laneRisk = lane;
        option.receiverPressure = pressure;
        option.executionDifficulty = difficulty;
        return option;
    }

    void pushTopOptions(
        std::vector<PassOption>& output,
        std::vector<PassOption> options,
        std::size_t limit) {
        std::sort(
            options.begin(),
            options.end(),
            [](const PassOption& lhs, const PassOption& rhs) {
                return lhs.score > rhs.score;
            });
        if (options.size() > limit) {
            options.resize(limit);
        }

        output.insert(output.end(), options.begin(), options.end());
    }
}

std::vector<PassOption> PassOptionEvaluator::evaluate(
    const PassOptionEvaluationContext& context) const {
    std::vector<PassOption> output;
    if (context.teamState == nullptr
        || context.opponentState == nullptr
        || context.carrierState == nullptr) {
        return output;
    }

    const PlayerAttributes passerAttributes =
        attributesFor(context.teamSnapshot, context.carrierState);
    const double skill = passingSkill(passerAttributes);
    const RoleRiskProfile role = passRoleRiskProfile(context.carrierRole);
    const TacticalBiasProfile tactics = passTacticalBiasProfile(context.tacticalSetup);
    const PassDecisionTuning tuning = passTuning();

    std::vector<PassOption> safeOptions;
    std::vector<PassOption> backOptions;
    std::vector<PassOption> progressiveOptions;
    std::vector<PassOption> switchOptions;
    std::vector<PassOption> throughOptions;
    std::vector<PassOption> crossOptions;
    std::vector<PassOption> cutbackOptions;

    const bool carrierWide = isWide(context.ballPosition);
    const bool carrierFinalThird = isFinalThird(context.ballPosition, context.attackingDirection);
    const bool allowCrossOrCutback =
        carrierWide && carrierFinalThird && context.carrierRole != FormationSlotRole::Goalkeeper;

    for (const PlayerSimState& teammate : context.teamState->players) {
        if (teammate.playerId == context.carrierState->playerId) {
            continue;
        }

        const double distance = PitchGeometry::distance(context.ballPosition, teammate.position);
        const double forward = forwardMeters(
            context.ballPosition,
            teammate.position,
            context.attackingDirection);
        const FormationSlotRole receiverRole = roleForPlayer(context.teamSnapshot, teammate.playerId);

        if (distance <= 32.0
            && (!isAhead(context.ballPosition, teammate.position, context.attackingDirection)
                || forward <= 10.0)) {
            safeOptions.push_back(makeOption(
                PassOptionKind::SafePass,
                context,
                teammate,
                teammate.position,
                receiverRole,
                passerAttributes,
                role,
                tactics,
                tuning));
        }

        if (distance <= 34.0 && isBehind(context.ballPosition, teammate.position, context.attackingDirection)) {
            backOptions.push_back(makeOption(
                PassOptionKind::BackPass,
                context,
                teammate,
                teammate.position,
                receiverRole,
                passerAttributes,
                role,
                tactics,
                tuning));
        }

        if (forward >= 5.0
            && distance >= 8.0
            && distance <= 42.0
            && (skill >= 38.0 || distance <= 25.0 || tactics.verticalityBias > 1.08)) {
            progressiveOptions.push_back(makeOption(
                PassOptionKind::ProgressivePass,
                context,
                teammate,
                teammate.position,
                receiverRole,
                passerAttributes,
                role,
                tactics,
                tuning));
        }

        if (distance >= 26.0
            && distance <= 64.0
            && isOppositeWideSide(context.ballPosition, teammate.position)
            && (isWide(teammate.position) || isFullbackOrWideRole(receiverRole))) {
            switchOptions.push_back(makeOption(
                PassOptionKind::SwitchPlay,
                context,
                teammate,
                teammate.position,
                receiverRole,
                passerAttributes,
                role,
                tactics,
                tuning));
        }

        if (context.carrierRole != FormationSlotRole::Goalkeeper
            && forward >= 10.0
            && distance >= 12.0
            && distance <= 44.0
            && isAttackingHalf(teammate.position, context.attackingDirection)
            && skill >= 55.0
            && (context.tacticalSetup.passingDirectness == PassingDirectness::Direct
                || context.tacticalSetup.mentality == TeamMentality::Attacking)) {
            throughOptions.push_back(makeOption(
                PassOptionKind::ThroughBall,
                context,
                teammate,
                throughBallTarget(teammate.position, context.attackingDirection),
                receiverRole,
                passerAttributes,
                role,
                tactics,
                tuning));
        }

        if (allowCrossOrCutback && distance <= 42.0) {
            const bool receiverInBox = isInOpponentBox(teammate.position, context.attackingDirection);
            const bool receiverCentral = std::abs(teammate.position.y - PitchGeometry::WidthMeters / 2.0)
                <= PitchGeometry::WidthMeters * 0.28;
            if (receiverInBox || (receiverCentral && isFinalThird(teammate.position, context.attackingDirection))) {
                crossOptions.push_back(makeOption(
                    PassOptionKind::Cross,
                    context,
                    teammate,
                    lowCrossTarget(context.attackingDirection),
                    receiverRole,
                    passerAttributes,
                    role,
                    tactics,
                    tuning));
            }

            const bool receiverBehindCarrier =
                forwardMeters(context.ballPosition, teammate.position, context.attackingDirection) < -1.0;
            if (receiverCentral && receiverBehindCarrier) {
                cutbackOptions.push_back(makeOption(
                    PassOptionKind::Cutback,
                    context,
                    teammate,
                    cutbackTarget(context.ballPosition, context.attackingDirection),
                    receiverRole,
                    passerAttributes,
                    role,
                    tactics,
                    tuning));
            }
        }
    }

    pushTopOptions(output, safeOptions, 3);
    pushTopOptions(output, backOptions, 2);
    pushTopOptions(output, progressiveOptions, 3);
    pushTopOptions(output, switchOptions, 1);
    pushTopOptions(output, throughOptions, 2);
    pushTopOptions(output, crossOptions, 1);
    pushTopOptions(output, cutbackOptions, 1);

    std::sort(
        output.begin(),
        output.end(),
        [](const PassOption& lhs, const PassOption& rhs) {
            return lhs.score > rhs.score;
        });
    return output;
}
