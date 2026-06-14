#include"fm/match_engine/offball/OffBallSupportModel.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>

namespace {
    double attackSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    double progressToX(double progress, AttackingDirection direction) {
        const double x = std::clamp(progress, 0.0, PitchGeometry::LengthMeters);
        return direction == AttackingDirection::HomeToAway
            ? x
            : PitchGeometry::LengthMeters - x;
    }

    double progressOf(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }

    bool isFullback(FormationSlotRole role) {
        return role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack;
    }

    bool isWinger(FormationSlotRole role) {
        return role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger
            || role == FormationSlotRole::LeftMidfielder
            || role == FormationSlotRole::RightMidfielder;
    }

    bool isCentralMidfielder(FormationSlotRole role) {
        return role == FormationSlotRole::DefensiveMidfielder
            || role == FormationSlotRole::CentralMidfielder
            || role == FormationSlotRole::AttackingMidfielder;
    }

    bool isCenterBack(FormationSlotRole role) {
        return role == FormationSlotRole::CenterBack;
    }

    bool isDm(FormationSlotRole role) {
        return role == FormationSlotRole::DefensiveMidfielder;
    }

    bool hasActiveEvent(const OffBallSupportModelRequest& request, PlayerId playerId) {
        for (const OffBallSupportEvent& event : request.activeEvents) {
            if (event.playerId == playerId) {
                return true;
            }
        }
        return false;
    }

    const PlayerSimState* simPlayerFor(const TeamSimState& team, PlayerId playerId) {
        for (const PlayerSimState& player : team.players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }
        return nullptr;
    }

    PlayerAttributes attributesFor(const MatchTeamSnapshot* snapshot, PlayerId playerId) {
        if (snapshot == nullptr) {
            return PlayerAttributes{};
        }
        for (const MatchPlayerSnapshot& player : snapshot->players) {
            if (player.playerId == playerId) {
                return player.attributes;
            }
        }
        return PlayerAttributes{};
    }

    SupportRegion regionFromProgress(
        AttackingDirection direction,
        double minProgress,
        double maxProgress,
        double minY,
        double maxY,
        SupportRegionLane lane,
        SupportRegionDepth depth,
        FormationSlotRole role,
        double suitability = 1.0) {
        return clampSupportRegion(SupportRegion{
            progressToX(minProgress, direction),
            progressToX(maxProgress, direction),
            minY,
            maxY,
            lane,
            depth,
            role,
            suitability,
            direction
        });
    }

    SupportRegion wideLaneRegion(
        AttackingDirection direction,
        bool rightSide,
        double minProgress,
        double maxProgress,
        SupportRegionDepth depth,
        FormationSlotRole role) {
        const double minY = rightSide
            ? PitchGeometry::WidthMeters * 0.76
            : PitchGeometry::WidthMeters * 0.03;
        const double maxY = rightSide
            ? PitchGeometry::WidthMeters * 0.97
            : PitchGeometry::WidthMeters * 0.24;
        return regionFromProgress(
            direction,
            minProgress,
            maxProgress,
            minY,
            maxY,
            rightSide ? SupportRegionLane::Right : SupportRegionLane::Left,
            depth,
            role);
    }

    SupportRegion halfSpaceRegion(
        AttackingDirection direction,
        bool rightSide,
        double minProgress,
        double maxProgress,
        SupportRegionDepth depth,
        FormationSlotRole role) {
        const double minY = rightSide
            ? PitchGeometry::WidthMeters * 0.55
            : PitchGeometry::WidthMeters * 0.30;
        const double maxY = rightSide
            ? PitchGeometry::WidthMeters * 0.70
            : PitchGeometry::WidthMeters * 0.45;
        return regionFromProgress(
            direction,
            minProgress,
            maxProgress,
            minY,
            maxY,
            rightSide ? SupportRegionLane::HalfSpaceRight : SupportRegionLane::HalfSpaceLeft,
            depth,
            role);
    }

    SupportRegion centralRegion(
        AttackingDirection direction,
        double minProgress,
        double maxProgress,
        double widthShare,
        SupportRegionDepth depth,
        FormationSlotRole role) {
        const double centerY = PitchGeometry::WidthMeters / 2.0;
        const double halfWidth = PitchGeometry::WidthMeters * widthShare / 2.0;
        return regionFromProgress(
            direction,
            minProgress,
            maxProgress,
            centerY - halfWidth,
            centerY + halfWidth,
            SupportRegionLane::Center,
            depth,
            role);
    }

    OffBallSupportEvent makeEvent(
        const OffBallSupportModelRequest& request,
        const PlayerGameContext& player,
        OffBallEventType type,
        SupportRegion region,
        const char* reason,
        double priority,
        double duration,
        bool restDefenseSafety = false) {
        OffBallSupportEvent event;
        event.playerId = player.playerId;
        event.teamId = player.teamId;
        event.eventType = type;
        event.sourcePhase = request.teamContext.currentPhase;
        event.sourceReason = reason;
        event.startSecond = request.currentSecond;
        event.maxDurationSeconds = duration;
        event.targetRegion = region;
        event.resolvedTargetPoint = region.center();
        event.priority = priority;
        event.requiresRestDefenseSafety = restDefenseSafety;
        event.expiresOnPhaseChange = type != OffBallEventType::BackPassSupport
            && type != OffBallEventType::WideSupport;
        return event;
    }

    bool ballOnRight(const TeamGameContext& context) {
        return context.ballFlank == BallFlank::Right;
    }

    bool ballOnLeft(const TeamGameContext& context) {
        return context.ballFlank == BallFlank::Left;
    }

    bool isRightRole(FormationSlotRole role) {
        return role == FormationSlotRole::RightBack
            || role == FormationSlotRole::RightWingBack
            || role == FormationSlotRole::RightMidfielder
            || role == FormationSlotRole::RightWinger;
    }

    bool roleOnBallSide(FormationSlotRole role, const TeamGameContext& context) {
        if (context.ballFlank == BallFlank::Center) {
            return false;
        }
        return isRightRole(role) == ballOnRight(context);
    }

    bool aggressivePhase(MatchTeamPhase phase) {
        return phase == MatchTeamPhase::FinalizingPosition
            || phase == MatchTeamPhase::CounterAttack;
    }

    PlayerIntentType intentTypeForEvent(OffBallEventType type) {
        switch (type) {
        case OffBallEventType::OverlapRun:
        case OffBallEventType::WideSupport:
            return PlayerIntentType::OccupyWidth;
        case OffBallEventType::UnderlapRun:
        case OffBallEventType::CutInsideRun:
        case OffBallEventType::HalfSpaceSupport:
            return PlayerIntentType::OccupyHalfSpace;
        case OffBallEventType::FarPostRun:
            return PlayerIntentType::AttackFarPost;
        case OffBallEventType::NearPostRun:
            return PlayerIntentType::AttackNearPost;
        case OffBallEventType::PenaltySpotRun:
        case OffBallEventType::EdgeOfBoxSupport:
            return PlayerIntentType::AttackCutbackZone;
        case OffBallEventType::CounterForwardRun:
            return PlayerIntentType::MakeRunBehind;
        case OffBallEventType::BackPassSupport:
            return PlayerIntentType::DropForPass;
        case OffBallEventType::RestDefenseHold:
            return PlayerIntentType::HoldAttackingShape;
        case OffBallEventType::DefensiveRecoveryRun:
            return PlayerIntentType::RecoverToGoal;
        case OffBallEventType::None:
            return PlayerIntentType::None;
        }
        return PlayerIntentType::MoveToSupport;
    }
}

OffBallSupportModelResult OffBallSupportModel::evaluate(
    const OffBallSupportModelRequest& request) const {
    OffBallSupportModelResult result;
    if (request.team == nullptr || request.opponent == nullptr || !request.teamContext.hasPossession) {
        return result;
    }

    const RestDefenseSnapshot before = restDefenseModel_.evaluate(RestDefenseEvaluationRequest{
        request.team,
        &request.playerContexts,
        request.activeEvents,
        request.teamContext.ballPosition,
        request.attackingDirection
    });
    result.restDefenseStableBeforeSupport = before.stable;

    const double ballProgress = progressOf(request.teamContext.ballPosition, request.attackingDirection);
    const bool advancedBall = ballProgress >= PitchGeometry::LengthMeters * 0.58;
    const bool finalThirdBall = ballProgress >= PitchGeometry::LengthMeters * 0.66;
    const bool rightSideBall = request.teamContext.ballFlank == BallFlank::Right;

    std::vector<OffBallSupportEvent> candidates;
    candidates.reserve(request.playerContexts.size());

    for (const PlayerGameContext& playerContext : request.playerContexts) {
        if (hasActiveEvent(request, playerContext.playerId)) {
            continue;
        }
        const PlayerSimState* player = simPlayerFor(*request.team, playerContext.playerId);
        if (player == nullptr || player->hasBall) {
            continue;
        }

        if (request.teamContext.currentPhase == MatchTeamPhase::SettledDefense) {
            continue;
        }

        if (request.teamContext.currentPhase == MatchTeamPhase::DefensiveTransition) {
            continue;
        }

        if (request.teamContext.currentPhase == MatchTeamPhase::BuildUp) {
            if (isDm(playerContext.role)
                && playerContext.distanceToBall <= 24.0
                && request.teamContext.nearestSupportCount < 2) {
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    OffBallEventType::BackPassSupport,
                    centralRegion(
                        request.attackingDirection,
                        std::max(12.0, ballProgress - 16.0),
                        std::max(20.0, ballProgress - 5.0),
                        0.36,
                        SupportRegionDepth::BackSupport,
                        playerContext.role),
                    "build-up recycle triangle",
                    35.0,
                    7.0));
            } else if (isFullback(playerContext.role) && advancedBall) {
                const bool rightRole = isRightRole(playerContext.role);
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    OffBallEventType::WideSupport,
                    wideLaneRegion(
                        request.attackingDirection,
                        rightRole,
                        std::max(30.0, ballProgress - 8.0),
                        std::min(PitchGeometry::LengthMeters - 18.0, ballProgress + 10.0),
                        SupportRegionDepth::BackSupport,
                        playerContext.role),
                    "safe wide outlet",
                    32.0,
                    7.0));
            }
            continue;
        }

        if (request.teamContext.currentPhase == MatchTeamPhase::CounterAttack) {
            if (isWinger(playerContext.role) || playerContext.role == FormationSlotRole::Striker) {
                const bool rightRole = isRightRole(playerContext.role);
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    OffBallEventType::CounterForwardRun,
                    playerContext.role == FormationSlotRole::Striker
                        ? centralRegion(
                            request.attackingDirection,
                            std::min(PitchGeometry::LengthMeters - 18.0, ballProgress + 14.0),
                            std::min(PitchGeometry::LengthMeters - 7.0, ballProgress + 31.0),
                            0.34,
                            SupportRegionDepth::Box,
                            playerContext.role)
                        : halfSpaceRegion(
                            request.attackingDirection,
                            rightRole,
                            std::min(PitchGeometry::LengthMeters - 22.0, ballProgress + 12.0),
                            std::min(PitchGeometry::LengthMeters - 7.0, ballProgress + 34.0),
                            SupportRegionDepth::Box,
                            playerContext.role),
                    "counter lane ahead",
                    62.0,
                    6.0));
            }
            continue;
        }

        if (request.teamContext.currentPhase != MatchTeamPhase::FinalizingPosition || !finalThirdBall) {
            continue;
        }

        if (isFullback(playerContext.role)) {
            if (playerContext.isFarSideFullback || before.farSideFullbackShouldHold) {
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    OffBallEventType::RestDefenseHold,
                    regionFromProgress(
                        request.attackingDirection,
                        std::max(24.0, ballProgress - 28.0),
                        std::max(36.0, ballProgress - 12.0),
                        isRightRole(playerContext.role)
                            ? PitchGeometry::WidthMeters * 0.62
                            : PitchGeometry::WidthMeters * 0.18,
                        isRightRole(playerContext.role)
                            ? PitchGeometry::WidthMeters * 0.88
                            : PitchGeometry::WidthMeters * 0.38,
                        isRightRole(playerContext.role)
                            ? SupportRegionLane::Right
                            : SupportRegionLane::Left,
                        SupportRegionDepth::BackSupport,
                        playerContext.role),
                    "far-side fullback rest defense",
                    58.0,
                    8.0));
                continue;
            }

            const bool ballSide = roleOnBallSide(playerContext.role, request.teamContext);
            if (ballSide) {
                const OffBallEventType type =
                    request.teamContext.centralSpaceAvailable
                        ? OffBallEventType::UnderlapRun
                        : OffBallEventType::OverlapRun;
                if (!restDefenseModel_.canApproveAdvancedRun(before, playerContext, type)) {
                    ++result.rejectedByRestDefense;
                    continue;
                }
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    type,
                    type == OffBallEventType::OverlapRun
                        ? wideLaneRegion(
                            request.attackingDirection,
                            rightSideBall,
                            std::min(PitchGeometry::LengthMeters - 24.0, ballProgress + 5.0),
                            PitchGeometry::LengthMeters - 5.5,
                            SupportRegionDepth::Byline,
                            playerContext.role)
                        : halfSpaceRegion(
                            request.attackingDirection,
                            rightSideBall,
                            std::min(PitchGeometry::LengthMeters - 25.0, ballProgress + 3.0),
                            PitchGeometry::LengthMeters - 8.0,
                            SupportRegionDepth::Box,
                            playerContext.role),
                    type == OffBallEventType::OverlapRun
                        ? "ball-side wide overlap"
                        : "ball-side half-space underlap",
                    64.0,
                    7.0,
                    true));
            }
            continue;
        }

        if (isWinger(playerContext.role)) {
            const bool rightRole = isRightRole(playerContext.role);
            if (playerContext.isFarSideWinger) {
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    OffBallEventType::FarPostRun,
                    regionFromProgress(
                        request.attackingDirection,
                        PitchGeometry::LengthMeters - 17.0,
                        PitchGeometry::LengthMeters - 6.0,
                        rightRole
                            ? PitchGeometry::WidthMeters * 0.56
                            : PitchGeometry::WidthMeters * 0.20,
                        rightRole
                            ? PitchGeometry::WidthMeters * 0.78
                            : PitchGeometry::WidthMeters * 0.44,
                        rightRole ? SupportRegionLane::Right : SupportRegionLane::Left,
                        SupportRegionDepth::FarPost,
                        playerContext.role),
                    "far-side winger attacks far post",
                    61.0,
                    6.0));
            } else if (playerContext.isBallSideWinger && !playerContext.canSupportBallCarrier) {
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    OffBallEventType::CutInsideRun,
                    halfSpaceRegion(
                        request.attackingDirection,
                        rightRole,
                        std::min(PitchGeometry::LengthMeters - 25.0, ballProgress + 4.0),
                        PitchGeometry::LengthMeters - 8.0,
                        SupportRegionDepth::Edge,
                        playerContext.role),
                    "ball-side winger inside lane",
                    54.0,
                    6.0));
            }
            continue;
        }

        if (playerContext.role == FormationSlotRole::Striker) {
            const OffBallEventType type =
                request.teamContext.ballFlank == BallFlank::Center
                    ? OffBallEventType::PenaltySpotRun
                    : OffBallEventType::NearPostRun;
            candidates.push_back(makeEvent(
                request,
                playerContext,
                type,
                type == OffBallEventType::PenaltySpotRun
                    ? centralRegion(
                        request.attackingDirection,
                        PitchGeometry::LengthMeters - 18.0,
                        PitchGeometry::LengthMeters - 8.0,
                        0.25,
                        SupportRegionDepth::Box,
                        playerContext.role)
                    : regionFromProgress(
                        request.attackingDirection,
                        PitchGeometry::LengthMeters - 16.0,
                        PitchGeometry::LengthMeters - 7.0,
                        ballOnLeft(request.teamContext)
                            ? PitchGeometry::WidthMeters * 0.28
                            : PitchGeometry::WidthMeters * 0.44,
                        ballOnLeft(request.teamContext)
                            ? PitchGeometry::WidthMeters * 0.46
                            : PitchGeometry::WidthMeters * 0.62,
                        SupportRegionLane::Center,
                        SupportRegionDepth::NearPost,
                        playerContext.role),
                type == OffBallEventType::PenaltySpotRun
                    ? "striker central box run"
                    : "striker near-post run",
                66.0,
                6.0));
            continue;
        }

        if (isCentralMidfielder(playerContext.role)) {
            if (isDm(playerContext.role) && before.playersBehindBall < 4) {
                candidates.push_back(makeEvent(
                    request,
                    playerContext,
                    OffBallEventType::RestDefenseHold,
                    centralRegion(
                        request.attackingDirection,
                        std::max(26.0, ballProgress - 24.0),
                        std::max(38.0, ballProgress - 10.0),
                        0.38,
                        SupportRegionDepth::BackSupport,
                        playerContext.role),
                    "dm rest-defense recycle hold",
                    46.0,
                    8.0));
                continue;
            }

            const bool rightHalfSpace =
                request.teamContext.ballFlank == BallFlank::Left
                    ? false
                    : request.teamContext.ballFlank == BallFlank::Right;
            candidates.push_back(makeEvent(
                request,
                playerContext,
                playerContext.role == FormationSlotRole::AttackingMidfielder
                    ? OffBallEventType::HalfSpaceSupport
                    : OffBallEventType::EdgeOfBoxSupport,
                playerContext.role == FormationSlotRole::AttackingMidfielder
                    ? halfSpaceRegion(
                        request.attackingDirection,
                        rightHalfSpace,
                        PitchGeometry::LengthMeters - 24.0,
                        PitchGeometry::LengthMeters - 14.0,
                        SupportRegionDepth::Edge,
                        playerContext.role)
                    : centralRegion(
                        request.attackingDirection,
                        PitchGeometry::LengthMeters - 25.0,
                        PitchGeometry::LengthMeters - 17.0,
                        0.34,
                        SupportRegionDepth::Edge,
                        playerContext.role),
                "midfield second-line support",
                50.0,
                6.5));
        }
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const OffBallSupportEvent& lhs, const OffBallSupportEvent& rhs) {
            if (lhs.priority == rhs.priority) {
                return lhs.playerId < rhs.playerId;
            }
            return lhs.priority > rhs.priority;
        });

    const std::size_t maxEvents =
        aggressivePhase(request.teamContext.currentPhase) ? 6 : 4;
    if (candidates.size() > maxEvents) {
        candidates.resize(maxEvents);
    }

    for (OffBallSupportEvent& event : candidates) {
        const PlayerSimState* player = simPlayerFor(*request.team, event.playerId);
        const PlayerGameContext* context = nullptr;
        for (const PlayerGameContext& candidateContext : request.playerContexts) {
            if (candidateContext.playerId == event.playerId) {
                context = &candidateContext;
                break;
            }
        }
        if (player == nullptr || context == nullptr) {
            continue;
        }
        event.startPosition = player->position;
        const OffBallTargetResolveResult target = targetResolver_.resolve(OffBallTargetResolveRequest{
            event,
            *player,
            *context,
            request.team->players,
            request.opponent->players,
            request.teamContext.ballPosition,
            request.attackingDirection,
            attributesFor(request.teamSnapshot, event.playerId),
            request.offsideLine,
            request.currentSecond
        });
        event.resolvedTargetPoint = target.targetPoint;
        event.distanceToTargetAtCreation =
            PitchGeometry::distance(event.startPosition, event.resolvedTargetPoint);
        event.offsideAwarenessChecked = target.offsideAwareness.checked;
        event.offsideAwarenessAdjusted = target.offsideAwareness.adjusted;
        event.offsideAwarenessFailedToAdjust = target.offsideAwareness.failedToAdjust;
        event.offsideAwarenessCheckInterval = target.offsideAwareness.checkIntervalSeconds;
        event.distanceToOffsideLineAtTarget = target.offsideAwareness.distanceToOffsideLine;
        result.events.push_back(event);
    }

    std::vector<OffBallSupportEvent> projectedEvents = request.activeEvents;
    projectedEvents.insert(projectedEvents.end(), result.events.begin(), result.events.end());
    const RestDefenseSnapshot after = restDefenseModel_.evaluate(RestDefenseEvaluationRequest{
        request.team,
        &request.playerContexts,
        projectedEvents,
        request.teamContext.ballPosition,
        request.attackingDirection
    });
    result.restDefenseStableAfterSupport = after.stable;
    return result;
}
