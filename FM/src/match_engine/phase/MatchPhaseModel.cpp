#include"fm/match_engine/phase/MatchPhaseModel.h"

#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/phase/SpaceContext.h"

#include<algorithm>
#include<cmath>

namespace {
    const PlayerShapeTarget* targetFor(
        const std::vector<PlayerShapeTarget>& targets,
        PlayerId playerId) {
        for (const PlayerShapeTarget& target : targets) {
            if (target.playerId == playerId) {
                return &target;
            }
        }
        return nullptr;
    }

    double averageShapeDistance(
        const TeamSimState& team,
        const std::vector<PlayerShapeTarget>& targets) {
        double total = 0.0;
        int samples = 0;
        for (const PlayerSimState& player : team.players) {
            const PlayerShapeTarget* target = targetFor(targets, player.playerId);
            if (target == nullptr) {
                continue;
            }
            total += PitchGeometry::distance(player.position, target->finalTarget);
            ++samples;
        }
        return samples > 0 ? total / static_cast<double>(samples) : 100.0;
    }

    bool roleIsFullback(FormationSlotRole role) {
        return role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack;
    }

    bool roleIsWinger(FormationSlotRole role) {
        return role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::RightWinger
            || role == FormationSlotRole::LeftMidfielder
            || role == FormationSlotRole::RightMidfielder;
    }

    bool roleIsCentralMidfielder(FormationSlotRole role) {
        return role == FormationSlotRole::DefensiveMidfielder
            || role == FormationSlotRole::CentralMidfielder
            || role == FormationSlotRole::AttackingMidfielder;
    }

    BallFlank flankFor(PitchPoint point) {
        const double third = PitchGeometry::WidthMeters / 3.0;
        if (point.y < third) {
            return BallFlank::Left;
        }
        if (point.y > third * 2.0) {
            return BallFlank::Right;
        }
        return BallFlank::Center;
    }

    bool sameFlank(PitchPoint point, BallFlank flank) {
        return flankFor(point) == flank || flank == BallFlank::Center;
    }

    AttackingDirection directionFor(const TeamSimState& team) {
        return team.side == TeamSide::Home
            ? AttackingDirection::HomeToAway
            : AttackingDirection::AwayToHome;
    }

    double forwardMeters(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? to.x - from.x
            : from.x - to.x;
    }

    int nearbyCount(
        const std::vector<PlayerSimState>& players,
        PitchPoint point,
        PlayerId excludedPlayerId,
        double radius) {
        int count = 0;
        for (const PlayerSimState& player : players) {
            if (player.playerId != excludedPlayerId
                && PitchGeometry::distance(player.position, point) <= radius) {
                ++count;
            }
        }
        return count;
    }

    FormationSlotRole assignedRoleForSheet(const TeamSheet& teamSheet, PlayerId playerId) {
        for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }
        return FormationSlotRole::Unknown;
    }
}

MatchPhaseModel::MatchPhaseModel(PhaseTransitionConfig config)
    : transitionModel_(config) {
}

PhaseTransitionResult MatchPhaseModel::evaluateTeamPhase(const TeamGameContext& context) const {
    return transitionModel_.evaluate(context);
}

TeamGameContext MatchPhaseModel::buildTeamContext(
    const TeamSimState& team,
    const TeamSimState& opponent,
    const MatchSimulationState& state,
    const std::vector<PlayerShapeTarget>& teamTargets,
    const std::vector<PlayerShapeTarget>& opponentTargets) const {
    const TeamId possessionTeamId = state.possession.teamInPossession != 0
        ? state.possession.teamInPossession
        : (state.ball.controlState == BallControlState::InFlight
            ? state.possession.lastPossessionTeamId
            : 0);

    const SpaceContextModel spaceModel;
    const SpaceContextSnapshot space = spaceModel.evaluate(team, opponent, state.ball.position);
    const SpaceContextSnapshot opponentSpace = spaceModel.evaluate(opponent, team, state.ball.position);

    TeamGameContext context;
    context.teamId = team.teamId;
    context.possessionTeamId = possessionTeamId;
    context.hasPossession = possessionTeamId == team.teamId;
    context.possessionRegained =
        state.ball.controlState == BallControlState::Controlled
        && state.possession.teamInPossession == team.teamId
        && state.possession.isTransition;
    context.currentPhase = team.currentPhase;
    context.previousPhase = team.previousPhase;
    context.ballPosition = state.ball.position;
    context.ballZone = space.ballZone;
    context.ballFlank = space.ballFlank;
    context.ballProgress = space.ballProgress;
    context.teamShapeSettled =
        averageShapeDistance(team, teamTargets) <= (context.hasPossession ? 10.0 : 8.0)
        || state.possession.actionDepth >= 3;
    context.opponentShapeSettled =
        averageShapeDistance(opponent, opponentTargets) <= 9.0
        && space.openForwardLaneScore < 62.0;
    context.supportAvailableNearBall = space.supportAvailableNearBall;
    context.openForwardLaneScore = space.openForwardLaneScore;
    context.openWideLaneScore = space.openWideLaneScore;
    context.centralSpaceAvailable = space.centralSpaceAvailable;
    context.wideSpaceAvailableLeft = space.wideSpaceAvailableLeft;
    context.wideSpaceAvailableRight = space.wideSpaceAvailableRight;
    context.counterOpportunityScore = space.counterOpportunityScore;
    context.counterThreatAgainstTeamScore = opponentSpace.counterThreatScore;
    context.restDefenseStable = space.restDefenseStable;
    context.playersAheadOfBall = space.playersAheadOfBall;
    context.playersBehindBall = space.playersBehindBall;
    context.nearestSupportCount = space.nearestSupportCount;
    context.possessionActionDepth = context.hasPossession ? state.possession.actionDepth : 0;
    context.secondsInCurrentPhase = static_cast<double>(
        std::max(0, state.currentSecond - team.phaseEntrySecond));
    context.phaseEntryReason = team.phaseEntryReason;
    context.phaseExitReason = team.phaseExitReason;
    return context;
}

std::vector<PlayerGameContext> MatchPhaseModel::buildPlayerContexts(
    const TeamSimState& team,
    const TeamSimState& opponent,
    const TeamSheet& teamSheet,
    const std::vector<PlayerShapeTarget>& teamTargets,
    PitchPoint ballPosition) const {
    std::vector<PlayerGameContext> contexts;
    contexts.reserve(team.players.size());

    const BallFlank ballFlank = flankFor(ballPosition);
    const AttackingDirection direction = directionFor(team);

    for (const PlayerSimState& player : team.players) {
        const FormationSlotRole role = assignedRoleForSheet(teamSheet, player.playerId);
        const PlayerShapeTarget* target = targetFor(teamTargets, player.playerId);
        const bool ahead = forwardMeters(ballPosition, player.position, direction) > 1.0;
        const bool behind = forwardMeters(player.position, ballPosition, direction) > -1.0;
        const bool playerSameFlank = sameFlank(player.position, ballFlank);

        PlayerGameContext context;
        context.playerId = player.playerId;
        context.teamId = player.teamId;
        context.role = role;
        context.currentPosition = player.position;
        context.assignedPosition = target != nullptr ? target->finalTarget : player.targetPosition;
        context.distanceToBall = PitchGeometry::distance(player.position, ballPosition);
        context.sameFlankAsBall = playerSameFlank;
        context.isBallSideFullback = roleIsFullback(role) && playerSameFlank;
        context.isFarSideFullback = roleIsFullback(role) && !playerSameFlank;
        context.isBallSideWinger = roleIsWinger(role) && playerSameFlank;
        context.isFarSideWinger = roleIsWinger(role) && !playerSameFlank;
        context.isCentralMidfielder = roleIsCentralMidfielder(role);
        context.isStriker = role == FormationSlotRole::Striker;
        context.inPrimaryZone =
            target != nullptr
            && PitchGeometry::distance(player.position, target->finalTarget) <= 8.0;
        context.inExtraZone =
            target != nullptr
            && PitchGeometry::distance(player.position, target->finalTarget) <= 14.0;
        context.aheadOfBall = ahead;
        context.behindBall = behind;
        context.canSupportBallCarrier = context.distanceToBall <= 18.0 && !player.hasBall;
        context.canJoinAttackWithoutBreakingRestDefense =
            ahead || (context.isCentralMidfielder && context.distanceToBall <= 24.0);
        context.shouldHoldRestDefense =
            role == FormationSlotRole::CenterBack
            || role == FormationSlotRole::DefensiveMidfielder
            || (roleIsFullback(role) && !playerSameFlank);
        context.nearbyOpponents = nearbyCount(opponent.players, player.position, 0, 9.0);
        context.nearbyTeammates = nearbyCount(team.players, player.position, player.playerId, 11.0);
        context.nearestPassingLaneAvailable =
            context.canSupportBallCarrier && context.nearbyOpponents <= 1;
        contexts.push_back(context);
    }

    return contexts;
}
