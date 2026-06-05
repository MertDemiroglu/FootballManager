#include"fm/match_engine/decision/PlayerIntentResolver.h"

#include"fm/match_engine/ball/BallTrajectoryBuilder.h"
#include"fm/match_engine/decision/DefensiveResponsibility.h"
#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<limits>

namespace {
    constexpr double TackleRangeMeters = 2.0;
    constexpr double PressRangeMeters = 8.0;
    constexpr double ContainRangeMeters = 14.0;
    constexpr double CoverRangeMeters = 30.0;

    struct IntentCandidate {
        PlayerIntentType type = PlayerIntentType::None;
        PitchPoint target;
        PlayerId relatedPlayerId = 0;
        double urgency = 0.0;
        double score = 0.0;
    };

    struct RecoveryCandidate {
        PlayerId playerId = 0;
        double score = 0.0;
    };

    struct InterceptionIntentCandidate {
        PlayerId playerId = 0;
        PitchPoint target;
        double distanceToPath = 0.0;
        double score = 0.0;
    };

    double attackSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    double attackingProgress(PitchPoint point, AttackingDirection direction) {
        const PitchPoint clamped = PitchGeometry::clampToPitch(point);
        if (direction == AttackingDirection::HomeToAway) {
            return clamped.x / PitchGeometry::LengthMeters;
        }

        return (PitchGeometry::LengthMeters - clamped.x) / PitchGeometry::LengthMeters;
    }

    bool isLooseOrDeflected(const PlayerIntentResolutionContext& context) {
        if (context.teamMode == IntentTeamMode::NeutralBall
            || context.ballState.controlState == BallControlState::Loose) {
            return true;
        }

        return context.lastContestResult
            && (context.lastContestResult->ballBecomesLoose
                || context.lastContestResult->ballDeflected
                || context.lastContestResult->ballOutcome == ContestBallOutcome::BallLoose
                || context.lastContestResult->ballOutcome == ContestBallOutcome::BallDeflected);
    }

    FormationSlotRole roleFromAssignments(
        const std::vector<TeamSheetSlotAssignment>& assignments,
        PlayerId playerId) {
        for (const TeamSheetSlotAssignment& assignment : assignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }

        return FormationSlotRole::Unknown;
    }

    const PlayerShapeTarget* shapeTargetFor(
        const std::vector<PlayerShapeTarget>& targets,
        PlayerId playerId) {
        for (const PlayerShapeTarget& target : targets) {
            if (target.playerId == playerId) {
                return &target;
            }
        }

        return nullptr;
    }

    FormationSlotRole roleFor(
        const PlayerIntentResolutionContext& context,
        PlayerId playerId) {
        const FormationSlotRole assignmentRole =
            roleFromAssignments(context.teamAssignments, playerId);
        if (assignmentRole != FormationSlotRole::Unknown) {
            return assignmentRole;
        }

        if (const PlayerShapeTarget* target = shapeTargetFor(context.shapeTargets, playerId)) {
            return target->slotRole;
        }

        return FormationSlotRole::Unknown;
    }

    PitchPoint shapeTargetOrCurrent(
        const PlayerIntentResolutionContext& context,
        const PlayerSimState& player) {
        if (const PlayerShapeTarget* target = shapeTargetFor(context.shapeTargets, player.playerId)) {
            return PitchGeometry::clampToPitch(target->finalTarget);
        }

        return PitchGeometry::clampToPitch(player.targetPosition);
    }

    const PlayerSimState* opponentBallCarrier(
        const PlayerIntentResolutionContext& context) {
        if (context.ballState.carrierPlayerId == 0) {
            return nullptr;
        }

        for (const PlayerSimState& opponent : context.opponents) {
            if (opponent.playerId == context.ballState.carrierPlayerId) {
                return &opponent;
            }
        }

        return nullptr;
    }

    const PlayerSimState* nearestOpponentTo(
        PitchPoint point,
        const std::vector<PlayerSimState>& opponents) {
        const PlayerSimState* nearest = nullptr;
        double bestDistance = std::numeric_limits<double>::max();

        for (const PlayerSimState& opponent : opponents) {
            const double distance = PitchGeometry::distance(point, opponent.position);
            if (distance < bestDistance) {
                nearest = &opponent;
                bestDistance = distance;
            }
        }

        return nearest;
    }

    const PlayerSimState* nearestOpponentInResponsibility(
        const PlayerIntentResolutionContext& context,
        const DefensiveResponsibility& responsibility) {
        const PlayerSimState* nearest = nullptr;
        double bestDistance = std::numeric_limits<double>::max();

        for (const PlayerSimState& opponent : context.opponents) {
            const TacticalZone opponentZone =
                tacticalZoneForPoint(opponent.position, context.attackingDirection);
            if (!isZoneInResponsibility(responsibility, opponentZone)) {
                continue;
            }

            const double distance = PitchGeometry::distance(context.ballState.position, opponent.position);
            if (distance < bestDistance) {
                nearest = &opponent;
                bestDistance = distance;
            }
        }

        return nearest;
    }

    PitchPoint advanceFrom(PitchPoint point, AttackingDirection direction, double meters) {
        return PitchGeometry::clampToPitch(PitchPoint{
            point.x + (attackSign(direction) * meters),
            point.y
        });
    }

    PitchPoint ownGoalCenter(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::homeGoalCenter()
            : PitchGeometry::awayGoalCenter();
    }

    PitchPoint attackingBoxTarget(
        AttackingDirection direction,
        double yOffset,
        double depthFromGoal) {
        const double x = direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters - depthFromGoal
            : depthFromGoal;
        return PitchGeometry::clampToPitch(PitchPoint{
            x,
            (PitchGeometry::WidthMeters / 2.0) + yOffset
        });
    }

    PitchPoint supportTarget(
        const PlayerSimState& player,
        const PlayerIntentResolutionContext& context,
        double behindBallMeters,
        double lateralMeters) {
        const double direction = attackSign(context.attackingDirection);
        const double yDirection =
            player.position.y < PitchGeometry::WidthMeters / 2.0 ? -1.0 : 1.0;
        return PitchGeometry::clampToPitch(PitchPoint{
            context.ballState.position.x - (direction * behindBallMeters),
            context.ballState.position.y + (yDirection * lateralMeters)
        });
    }

    PitchPoint laneTarget(
        const PlayerSimState& player,
        PitchPoint baseTarget,
        double laneBiasMeters) {
        const double center = PitchGeometry::WidthMeters / 2.0;
        const double yDirection = player.position.y < center ? -1.0 : 1.0;
        return PitchGeometry::clampToPitch(PitchPoint{
            baseTarget.x,
            baseTarget.y + (yDirection * laneBiasMeters)
        });
    }

    PitchPoint compactTowardBall(PitchPoint shapeTarget, PitchPoint ballPosition, double compactness) {
        const double factor = std::clamp(compactness, 0.0, 1.0);
        return PitchGeometry::clampToPitch(PitchPoint{
            shapeTarget.x + (ballPosition.x - shapeTarget.x) * factor,
            shapeTarget.y + (ballPosition.y - shapeTarget.y) * factor
        });
    }

    double pressingIntensityBoost(PressingIntensity intensity) {
        switch (intensity) {
        case PressingIntensity::Low:
            return -8.0;
        case PressingIntensity::Normal:
            return 0.0;
        case PressingIntensity::High:
            return 8.0;
        }

        return 0.0;
    }

    double zonalPreferenceBoost(MarkingStyle markingStyle, PlayerIntentType type) {
        if (markingStyle == MarkingStyle::Zonal) {
            if (type == PlayerIntentType::HoldDefensiveShape
                || type == PlayerIntentType::CoverSpace
                || type == PlayerIntentType::BlockPassingLane) {
                return 6.0;
            }
            if (type == PlayerIntentType::MarkOpponent) {
                return -5.0;
            }
        }

        if (markingStyle == MarkingStyle::ManOriented) {
            if (type == PlayerIntentType::MarkOpponent
                || type == PlayerIntentType::PressBallCarrier) {
                return 5.0;
            }
        }

        return 0.0;
    }

    bool isGoalkeeper(FormationSlotRole role) {
        return role == FormationSlotRole::Goalkeeper;
    }

    bool trajectoryEntersOwnBox(
        const BallTrajectory& trajectory,
        AttackingDirection direction) {
        const std::vector<BallTrajectorySample> samples = sampleTrajectory(trajectory, 7);
        for (const BallTrajectorySample& sample : samples) {
            if (direction == AttackingDirection::HomeToAway
                && PitchGeometry::isInsideHomePenaltyArea(sample.point)) {
                return true;
            }
            if (direction == AttackingDirection::AwayToHome
                && PitchGeometry::isInsideAwayPenaltyArea(sample.point)) {
                return true;
            }
        }

        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::isInsideHomePenaltyArea(trajectory.actualTarget)
            : PitchGeometry::isInsideAwayPenaltyArea(trajectory.actualTarget);
    }

    bool isCenterBackOrDm(FormationSlotRole role) {
        return role == FormationSlotRole::CenterBack
            || role == FormationSlotRole::DefensiveMidfielder;
    }

    bool isWideDefender(FormationSlotRole role) {
        return role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack;
    }

    bool isCentralMidfielder(FormationSlotRole role) {
        return role == FormationSlotRole::DefensiveMidfielder
            || role == FormationSlotRole::CentralMidfielder
            || role == FormationSlotRole::AttackingMidfielder;
    }

    bool isWinger(FormationSlotRole role) {
        return role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger
            || role == FormationSlotRole::LeftMidfielder
            || role == FormationSlotRole::RightMidfielder;
    }

    IntentCandidate bestCandidate(const std::vector<IntentCandidate>& candidates) {
        return *std::max_element(
            candidates.begin(),
            candidates.end(),
            [](const IntentCandidate& lhs, const IntentCandidate& rhs) {
                if (lhs.score == rhs.score) {
                    return static_cast<int>(lhs.type) > static_cast<int>(rhs.type);
                }
                return lhs.score < rhs.score;
            });
    }

    IntentCandidate fallbackCandidate(
        const PlayerIntentResolutionContext& context,
        const PlayerSimState& player,
        IntentTeamMode mode) {
        const PitchPoint target = shapeTargetOrCurrent(context, player);
        if (mode == IntentTeamMode::Attacking) {
            return IntentCandidate{
                PlayerIntentType::HoldAttackingShape,
                target,
                0,
                0.2,
                40.0
            };
        }

        if (mode == IntentTeamMode::NeutralBall) {
            return IntentCandidate{
                PlayerIntentType::CoverSpace,
                target,
                0,
                0.25,
                38.0
            };
        }

        return IntentCandidate{
            PlayerIntentType::HoldDefensiveShape,
            target,
            0,
            0.2,
            42.0 + zonalPreferenceBoost(context.tacticalSetup.markingStyle, PlayerIntentType::HoldDefensiveShape)
        };
    }

    IntentCandidate attackingIntent(
        const PlayerIntentResolutionContext& context,
        const PlayerSimState& player,
        FormationSlotRole role) {
        std::vector<IntentCandidate> candidates;
        const PitchPoint shapeTarget = shapeTargetOrCurrent(context, player);

        candidates.push_back(IntentCandidate{
            PlayerIntentType::HoldAttackingShape,
            shapeTarget,
            0,
            0.2,
            isGoalkeeper(role) ? 80.0 : 34.0
        });

        if (player.hasBall || context.ballState.carrierPlayerId == player.playerId) {
            return candidates.front();
        }

        if (role == FormationSlotRole::CenterBack) {
            candidates.front().score += 26.0;
            candidates.push_back(IntentCandidate{
                PlayerIntentType::MoveToSupport,
                shapeTarget,
                context.ballState.carrierPlayerId,
                0.25,
                34.0
            });
            return bestCandidate(candidates);
        }

        if (isWideDefender(role)) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::OccupyWidth,
                laneTarget(player, shapeTarget, 5.0),
                0,
                0.35,
                54.0
            });
            candidates.push_back(IntentCandidate{
                PlayerIntentType::MoveToSupport,
                supportTarget(player, context, 8.0, 7.0),
                context.ballState.carrierPlayerId,
                0.45,
                50.0
            });
        } else if (isCentralMidfielder(role)) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::MoveToSupport,
                supportTarget(player, context, 8.0, 4.0),
                context.ballState.carrierPlayerId,
                0.45,
                55.0
            });
            candidates.push_back(IntentCandidate{
                PlayerIntentType::DropForPass,
                supportTarget(player, context, 14.0, 3.0),
                context.ballState.carrierPlayerId,
                0.4,
                48.0
            });
            candidates.push_back(IntentCandidate{
                PlayerIntentType::OccupyHalfSpace,
                laneTarget(player, shapeTarget, 3.0),
                0,
                0.35,
                46.0
            });
        } else if (isWinger(role)) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::OccupyWidth,
                laneTarget(player, shapeTarget, 7.0),
                0,
                0.35,
                54.0
            });
            candidates.push_back(IntentCandidate{
                PlayerIntentType::MakeRunBehind,
                advanceFrom(player.position, context.attackingDirection, 13.0),
                0,
                0.7,
                52.0
            });
            candidates.push_back(IntentCandidate{
                PlayerIntentType::AttackFarPost,
                attackingBoxTarget(
                    context.attackingDirection,
                    player.position.y < PitchGeometry::WidthMeters / 2.0 ? 7.0 : -7.0,
                    6.0),
                0,
                0.65,
                attackingProgress(context.ballState.position, context.attackingDirection) > 0.66 ? 50.0 : 28.0
            });
        } else if (role == FormationSlotRole::Striker) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::MakeRunBehind,
                advanceFrom(player.position, context.attackingDirection, 10.0),
                0,
                0.75,
                58.0
            });
            candidates.push_back(IntentCandidate{
                PlayerIntentType::AttackNearPost,
                attackingBoxTarget(
                    context.attackingDirection,
                    player.position.y < PitchGeometry::WidthMeters / 2.0 ? -4.0 : 4.0,
                    5.0),
                0,
                0.8,
                attackingProgress(context.ballState.position, context.attackingDirection) > 0.66 ? 58.0 : 30.0
            });
            candidates.push_back(IntentCandidate{
                PlayerIntentType::AttackFarPost,
                attackingBoxTarget(
                    context.attackingDirection,
                    player.position.y < PitchGeometry::WidthMeters / 2.0 ? 6.0 : -6.0,
                    7.0),
                0,
                0.75,
                attackingProgress(context.ballState.position, context.attackingDirection) > 0.66 ? 52.0 : 28.0
            });
        }

        if (attackingProgress(context.ballState.position, context.attackingDirection) > 0.72
            && (isCentralMidfielder(role) || isWinger(role))) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::AttackCutbackZone,
                attackingBoxTarget(context.attackingDirection, 0.0, 16.0),
                0,
                0.55,
                44.0
            });
        }

        return bestCandidate(candidates);
    }

    IntentCandidate defendingIntent(
        const PlayerIntentResolutionContext& context,
        const PlayerSimState& player,
        FormationSlotRole role) {
        const PitchPoint shapeTarget = shapeTargetOrCurrent(context, player);
        const PitchPoint ballPosition = PitchGeometry::clampToPitch(context.ballState.position);
        const TacticalZone ballZone = tacticalZoneForPoint(ballPosition, context.attackingDirection);
        const DefensiveResponsibility responsibility = buildDefensiveResponsibility(
            player.playerId,
            role,
            context.attackingDirection);
        const PlayerSimState* carrier =
            opponentBallCarrier(context) != nullptr
                ? opponentBallCarrier(context)
                : nearestOpponentTo(ballPosition, context.opponents);
        const FormationSlotRole carrierRole = carrier == nullptr
            ? FormationSlotRole::Unknown
            : roleFromAssignments(context.opponentAssignments, carrier->playerId);
        const double distanceToCarrier = carrier == nullptr
            ? PitchGeometry::distance(player.position, ballPosition)
            : PitchGeometry::distance(player.position, carrier->position);
        const bool pressEligible = canConsiderPressing(
            responsibility,
            ballZone,
            carrierRole,
            distanceToCarrier,
            context.tacticalSetup.markingStyle);
        const bool laneRelevant = sameOrAdjacentLane(
            tacticalZoneForPoint(player.position, context.attackingDirection),
            ballZone);
        const bool contextPress =
            context.defensiveContext.pressTriggerActive
            && laneRelevant
            && !isGoalkeeper(role)
            && distanceToCarrier <= CoverRangeMeters
            && (isZoneInResponsibility(responsibility, ballZone) || laneRelevant);
        const double compactness =
            context.defensiveContext.blockCompactness >= 70.0 ? 0.08 : 0.18;
        const PitchPoint compactShapeTarget =
            compactTowardBall(shapeTarget, ballPosition, contextPress ? compactness : 0.0);
        const double contextPressureBoost =
            contextPress ? std::clamp(context.defensiveContext.localPressOpportunity * 0.22, 4.0, 16.0) : 0.0;

        std::vector<IntentCandidate> candidates;
        candidates.push_back(IntentCandidate{
            PlayerIntentType::HoldDefensiveShape,
            compactShapeTarget,
            0,
            0.2,
            38.0
                + (distanceToCarrier > CoverRangeMeters ? 16.0 : 0.0)
                + zonalPreferenceBoost(context.tacticalSetup.markingStyle, PlayerIntentType::HoldDefensiveShape)
        });

        if ((pressEligible || contextPress) && distanceToCarrier <= TackleRangeMeters) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::AttemptTackle,
                carrier != nullptr ? carrier->position : ballPosition,
                carrier != nullptr ? carrier->playerId : 0,
                1.0,
                70.0 + pressingIntensityBoost(context.tacticalSetup.pressingIntensity)
                    + contextPressureBoost
            });
        }

        if ((pressEligible && distanceToCarrier <= PressRangeMeters)
            || (contextPress && distanceToCarrier <= ContainRangeMeters)) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::PressBallCarrier,
                carrier != nullptr ? carrier->position : ballPosition,
                carrier != nullptr ? carrier->playerId : 0,
                0.85,
                58.0
                    + pressingIntensityBoost(context.tacticalSetup.pressingIntensity)
                    + zonalPreferenceBoost(context.tacticalSetup.markingStyle, PlayerIntentType::PressBallCarrier)
                    + contextPressureBoost
            });
        }

        if (distanceToCarrier <= ContainRangeMeters && laneRelevant) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::ContainBallCarrier,
                carrier != nullptr ? carrier->position : ballPosition,
                carrier != nullptr ? carrier->playerId : 0,
                0.65,
                pressEligible ? 50.0 : 56.0
                    + (context.tacticalSetup.pressingIntensity == PressingIntensity::Low ? 7.0 : 0.0)
                    + contextPressureBoost * 0.6
            });
        }

        if (distanceToCarrier <= CoverRangeMeters && laneRelevant) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::CoverSpace,
                compactShapeTarget,
                0,
                0.45,
                48.0
                    + zonalPreferenceBoost(context.tacticalSetup.markingStyle, PlayerIntentType::CoverSpace)
                    + contextPressureBoost * 0.5
            });
        }

        if (const PlayerSimState* markTarget =
            nearestOpponentInResponsibility(context, responsibility)) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::MarkOpponent,
                markTarget->position,
                markTarget->playerId,
                0.55,
                45.0
                    + zonalPreferenceBoost(context.tacticalSetup.markingStyle, PlayerIntentType::MarkOpponent)
            });
        }

        if (carrier != nullptr && laneRelevant) {
            const PitchPoint goal = ownGoalCenter(context.attackingDirection);
            candidates.push_back(IntentCandidate{
                PlayerIntentType::BlockPassingLane,
                PitchGeometry::clampToPitch(PitchPoint{
                    (carrier->position.x + goal.x) / 2.0,
                    (carrier->position.y + goal.y) / 2.0
                }),
                carrier->playerId,
                0.5,
                43.0
                    + zonalPreferenceBoost(context.tacticalSetup.markingStyle, PlayerIntentType::BlockPassingLane)
                    + contextPressureBoost * 0.75
            });
        }

        if (isCenterBackOrDm(role)
            && isDefensiveThird(ballZone)
            && isCentralZone(ballZone)) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::ProtectGoalZone,
                advanceFrom(ownGoalCenter(context.attackingDirection), context.attackingDirection, 13.0),
                0,
                0.7,
                56.0
            });
        }

        const double playerProgress = attackingProgress(player.position, context.attackingDirection);
        const double ballProgress = attackingProgress(ballPosition, context.attackingDirection);
        if (playerProgress - ballProgress > 0.22) {
            candidates.push_back(IntentCandidate{
                PlayerIntentType::RecoverToGoal,
                shapeTarget,
                0,
                0.75,
                54.0
            });
        }

        return bestCandidate(candidates);
    }

    double recoveryScoreFor(
        const PlayerIntentResolutionContext& context,
        const PlayerSimState& player,
        FormationSlotRole role) {
        const double distance = PitchGeometry::distance(player.position, context.ballState.position);
        const double pace = player.effectivePace > 0.0 ? player.effectivePace : 6.0;
        const double acceleration = player.effectiveAcceleration > 0.0
            ? player.effectiveAcceleration
            : pace;
        const TacticalZone playerZone = tacticalZoneForPoint(player.position, context.attackingDirection);
        const TacticalZone ballZone = tacticalZoneForPoint(context.ballState.position, context.attackingDirection);
        const DefensiveResponsibility responsibility = buildDefensiveResponsibility(
            player.playerId,
            role,
            context.attackingDirection);
        const bool zoneRelevant =
            isZoneInResponsibility(responsibility, ballZone)
            || sameOrAdjacentLane(playerZone, ballZone);
        const double roleBonus = zoneRelevant ? 12.0 : -8.0;
        const double distanceScore = std::max(0.0, 42.0 - (distance * 1.8));
        const double physicalScore = std::clamp((pace * 2.2) + acceleration, 0.0, 28.0);
        const double overallScore = std::clamp(static_cast<double>(player.baseOverall), 0.0, 100.0) * 0.1;
        const double intensityBoost =
            context.tacticalSetup.pressingIntensity == PressingIntensity::High ? 6.0 : 0.0;

        return distanceScore + physicalScore + overallScore + roleBonus + intensityBoost;
    }

    std::vector<RecoveryCandidate> recoveryCandidates(
        const PlayerIntentResolutionContext& context) {
        std::vector<RecoveryCandidate> candidates;
        candidates.reserve(context.teammates.size());

        for (const PlayerSimState& player : context.teammates) {
            candidates.push_back(RecoveryCandidate{
                player.playerId,
                recoveryScoreFor(context, player, roleFor(context, player.playerId))
            });
        }

        std::sort(
            candidates.begin(),
            candidates.end(),
            [](const RecoveryCandidate& lhs, const RecoveryCandidate& rhs) {
                if (lhs.score == rhs.score) {
                    return lhs.playerId < rhs.playerId;
                }
                return lhs.score > rhs.score;
            });

        return candidates;
    }

    bool selectedForRecovery(
        const std::vector<RecoveryCandidate>& candidates,
        PlayerId playerId) {
        constexpr std::size_t MaxRecoveringPlayers = 3;
        for (std::size_t i = 0; i < candidates.size() && i < MaxRecoveringPlayers; ++i) {
            if (candidates[i].playerId == playerId && candidates[i].score >= 28.0) {
                return true;
            }
        }

        return false;
    }

    bool responsibilityLaneRelevant(
        const DefensiveResponsibility& responsibility,
        TacticalZone zone) {
        if (responsibility.primaryZone != TacticalZone::Unknown
            && sameOrAdjacentLane(responsibility.primaryZone, zone)) {
            return true;
        }

        for (TacticalZone secondaryZone : responsibility.secondaryZones) {
            if (secondaryZone != TacticalZone::Unknown
                && sameOrAdjacentLane(secondaryZone, zone)) {
                return true;
            }
        }

        return false;
    }

    double distanceToTrajectoryActualTarget(
        const PlayerSimState& player,
        const BallTrajectory& trajectory) {
        return PitchGeometry::distance(player.position, trajectory.actualTarget);
    }

    InterceptionIntentCandidate interceptionCandidateFor(
        const PlayerIntentResolutionContext& context,
        const PlayerSimState& player,
        FormationSlotRole role,
        const std::vector<BallTrajectorySample>& samples,
        bool keeperRelevant) {
        InterceptionIntentCandidate candidate;
        candidate.playerId = player.playerId;

        if (samples.empty() || role == FormationSlotRole::Unknown) {
            candidate.score = -100.0;
            return candidate;
        }

        if (isGoalkeeper(role) && !keeperRelevant) {
            candidate.score = -100.0;
            return candidate;
        }

        PitchPoint bestPoint = samples.front().point;
        double bestDistance = std::numeric_limits<double>::max();
        for (const BallTrajectorySample& sample : samples) {
            const double distance = PitchGeometry::distance(player.position, sample.point);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestPoint = sample.point;
            }
        }

        const TacticalZone playerZone =
            tacticalZoneForPoint(player.position, context.attackingDirection);
        const TacticalZone sampleZone =
            tacticalZoneForPoint(bestPoint, context.attackingDirection);
        const DefensiveResponsibility responsibility = buildDefensiveResponsibility(
            player.playerId,
            role,
            context.attackingDirection);
        const bool zoneResponsible = isZoneInResponsibility(responsibility, sampleZone);
        const bool laneRelevant =
            sameOrAdjacentLane(playerZone, sampleZone)
            || responsibilityLaneRelevant(responsibility, sampleZone);
        const double pace = player.effectivePace > 0.0 ? player.effectivePace : 6.0;
        const double targetDistance =
            context.ballState.trajectory
                ? distanceToTrajectoryActualTarget(player, *context.ballState.trajectory)
                : bestDistance;

        double score = 0.0;
        score += std::max(0.0, 46.0 - (bestDistance * 4.5));
        score += std::max(0.0, 18.0 - (targetDistance * 0.45));
        score += sameVerticalLane(playerZone, sampleZone) ? 14.0 : 0.0;
        score += adjacentLane(playerZone, sampleZone) ? 8.0 : 0.0;
        score += zoneResponsible ? 15.0 : 0.0;
        score += !zoneResponsible && responsibilityLaneRelevant(responsibility, sampleZone) ? 8.0 : 0.0;
        score += laneRelevant ? 4.0 : -12.0;
        score += std::clamp(pace * 1.8, 0.0, 16.0);
        score += std::clamp(static_cast<double>(player.baseOverall), 0.0, 100.0) * 0.08;

        if (bestDistance > 18.0) {
            score -= 18.0;
        }
        if (isGoalkeeper(role)) {
            score += keeperRelevant ? 10.0 : -100.0;
        }

        candidate.target = PitchGeometry::clampToPitch(bestPoint);
        candidate.distanceToPath = bestDistance;
        candidate.score = score;
        return candidate;
    }

    std::vector<InterceptionIntentCandidate> interceptionCandidates(
        const PlayerIntentResolutionContext& context) {
        if (context.teamMode != IntentTeamMode::Defending
            || context.ballState.controlState != BallControlState::InFlight
            || !context.ballState.trajectory) {
            return {};
        }

        const BallTrajectory& trajectory = *context.ballState.trajectory;
        const std::vector<BallTrajectorySample> samples = sampleTrajectory(trajectory, 7);
        const bool keeperRelevant =
            trajectoryEntersOwnBox(trajectory, context.attackingDirection);
        std::vector<InterceptionIntentCandidate> candidates;
        candidates.reserve(context.teammates.size());

        for (const PlayerSimState& player : context.teammates) {
            candidates.push_back(interceptionCandidateFor(
                context,
                player,
                roleFor(context, player.playerId),
                samples,
                keeperRelevant));
        }

        std::sort(
            candidates.begin(),
            candidates.end(),
            [](const InterceptionIntentCandidate& lhs, const InterceptionIntentCandidate& rhs) {
                if (lhs.score == rhs.score) {
                    return lhs.playerId < rhs.playerId;
                }
                return lhs.score > rhs.score;
            });

        return candidates;
    }

    const InterceptionIntentCandidate* selectedInterceptionFor(
        const std::vector<InterceptionIntentCandidate>& candidates,
        PlayerId playerId) {
        constexpr std::size_t MaxInterceptors = 3;
        constexpr double MinimumInterceptionScore = 42.0;
        for (std::size_t i = 0; i < candidates.size() && i < MaxInterceptors; ++i) {
            if (candidates[i].playerId == playerId
                && candidates[i].score >= MinimumInterceptionScore) {
                return &candidates[i];
            }
        }

        return nullptr;
    }
}

std::vector<ResolvedPlayerIntent> PlayerIntentResolver::resolveTeamIntents(
    const PlayerIntentResolutionContext& context) const {
    std::vector<ResolvedPlayerIntent> resolved;
    resolved.reserve(context.teammates.size());

    const bool looseOrDeflected = isLooseOrDeflected(context);
    const std::vector<RecoveryCandidate> recoveries = looseOrDeflected
        ? recoveryCandidates(context)
        : std::vector<RecoveryCandidate>{};
    const std::vector<InterceptionIntentCandidate> interceptions =
        !looseOrDeflected ? interceptionCandidates(context) : std::vector<InterceptionIntentCandidate>{};

    for (const PlayerSimState& player : context.teammates) {
        const FormationSlotRole role = roleFor(context, player.playerId);
        IntentCandidate selected;

        if (looseOrDeflected && selectedForRecovery(recoveries, player.playerId)) {
            const double distance = PitchGeometry::distance(player.position, context.ballState.position);
            selected = IntentCandidate{
                PlayerIntentType::RecoverLooseBall,
                PitchGeometry::clampToPitch(context.ballState.position),
                0,
                distance <= PressRangeMeters ? 1.0 : 0.8,
                72.0 - std::min(distance, 28.0)
            };
        } else if (const InterceptionIntentCandidate* interception =
            selectedInterceptionFor(interceptions, player.playerId)) {
            selected = IntentCandidate{
                PlayerIntentType::InterceptBallPath,
                interception->target,
                0,
                interception->distanceToPath <= PressRangeMeters ? 1.0 : 0.85,
                interception->score
            };
        } else if (context.teamMode == IntentTeamMode::Attacking) {
            selected = attackingIntent(context, player, role);
        } else if (context.teamMode == IntentTeamMode::Defending) {
            selected = defendingIntent(context, player, role);
        } else {
            selected = fallbackCandidate(context, player, IntentTeamMode::NeutralBall);
        }

        PlayerIntent intent;
        intent.type = selected.type;
        intent.target = PitchGeometry::clampToPitch(selected.target);
        intent.relatedPlayerId = selected.relatedPlayerId;
        intent.urgency = std::clamp(selected.urgency, 0.0, 1.0);

        const PitchPoint finalTarget = PitchGeometry::clampToPitch(selected.target);
        resolved.push_back(ResolvedPlayerIntent{
            player.playerId,
            player.teamId,
            intent,
            finalTarget,
            tacticalZoneForPoint(player.position, context.attackingDirection),
            tacticalZoneForPoint(finalTarget, context.attackingDirection),
            selected.score
        });
    }

    return resolved;
}
