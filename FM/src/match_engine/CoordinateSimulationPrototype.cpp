#include"fm/match_engine/CoordinateSimulationPrototype.h"

#include"DeterministicRandom.h"
#include"fm/match_engine/ActionCandidateGenerator.h"
#include"fm/match_engine/ActionSelector.h"
#include"fm/match_engine/BallTrajectoryBuilder.h"
#include"fm/match_engine/ContestResolver.h"
#include"fm/match_engine/InterceptionResolver.h"
#include"fm/match_engine/MovementResolver.h"
#include"fm/match_engine/PitchGeometry.h"
#include"fm/match_engine/PlayerIntentResolver.h"
#include"fm/match_engine/TeamShapeModel.h"

#include<algorithm>
#include<cmath>
#include<cstdint>
#include<limits>
#include<optional>
#include<stdexcept>
#include<vector>

namespace {
    constexpr double LooseBallControlRangeMeters = 1.5;
    constexpr double ReceptionControlRangeMeters = 2.5;

    struct PendingBallAction {
        TeamId sourceTeamId = 0;
        PlayerId sourcePlayerId = 0;
        PlayerId targetPlayerId = 0;
        BallCarrierActionType actionType = BallCarrierActionType::Hold;
        BallTrajectoryType trajectoryType = BallTrajectoryType::GroundPass;
        bool isShot = false;
    };

    PitchPoint pitchCenter() {
        return PitchPoint{
            PitchGeometry::LengthMeters / 2.0,
            PitchGeometry::WidthMeters / 2.0
        };
    }

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    std::uint64_t combineSeed(std::uint64_t seed, std::uint64_t value) {
        return matchEngineMix64(seed ^ matchEngineMix64(value + 0x9e3779b97f4a7c15ULL));
    }

    std::uint64_t fallbackSeedFor(const MatchEngineInput& input) {
        std::uint64_t seed = 0x4d595df4d0f33173ULL;
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.matchId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.leagueId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.homeTeam.teamId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.awayTeam.teamId));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.matchDate.getYear()));
        seed = combineSeed(seed, static_cast<std::uint64_t>(static_cast<int>(input.matchDate.getMonth())));
        seed = combineSeed(seed, static_cast<std::uint64_t>(input.matchDate.getDay()));
        return seed == 0 ? 0x4d595df4d0f33173ULL : seed;
    }

    std::uint64_t baseSeedFor(const MatchEngineInput& input) {
        return input.options.deterministicSeed != 0
            ? input.options.deterministicSeed
            : fallbackSeedFor(input);
    }

    std::uint64_t stepSeed(
        std::uint64_t baseSeed,
        const MatchSimulationState& state,
        std::uint64_t salt) {
        std::uint64_t seed = combineSeed(baseSeed, static_cast<std::uint64_t>(state.currentSecond));
        seed = combineSeed(seed, static_cast<std::uint64_t>(state.possession.actionDepth));
        return combineSeed(seed, salt);
    }

    double deltaSecondsFor(const MatchEngineInput& input) {
        const double requested = input.options.detail == MatchSimulationDetail::BackgroundSummary
            ? input.options.backgroundStepSeconds
            : input.options.watchedStepSeconds;
        return clampDouble(requested, 0.25, 5.0);
    }

    int maxActionsFor(MatchSimulationDetail detail) {
        switch (detail) {
        case MatchSimulationDetail::BackgroundSummary:
            return 12;
        case MatchSimulationDetail::WatchedMatch:
            return 20;
        case MatchSimulationDetail::DebugFullTrace:
            return 30;
        }

        return 12;
    }

    bool shouldTrace(MatchSimulationDetail detail) {
        return detail != MatchSimulationDetail::BackgroundSummary;
    }

    const MatchPlayerSnapshot* findSnapshot(
        const MatchTeamSnapshot& team,
        PlayerId playerId) {
        for (const MatchPlayerSnapshot& player : team.players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }

        return nullptr;
    }

    const MatchTeamSnapshot* snapshotForTeam(
        const MatchEngineInput& input,
        TeamId teamId) {
        if (input.homeTeam.teamId == teamId) {
            return &input.homeTeam;
        }
        if (input.awayTeam.teamId == teamId) {
            return &input.awayTeam;
        }

        return nullptr;
    }

    const MatchPlayerSnapshot* findSnapshotForPlayer(
        const MatchEngineInput& input,
        PlayerId playerId) {
        if (const MatchPlayerSnapshot* player = findSnapshot(input.homeTeam, playerId)) {
            return player;
        }

        return findSnapshot(input.awayTeam, playerId);
    }

    const TeamSimState* opponentTeam(
        const MatchSimulationState& state,
        TeamId teamId) {
        if (state.homeTeam.teamId == teamId) {
            return &state.awayTeam;
        }
        if (state.awayTeam.teamId == teamId) {
            return &state.homeTeam;
        }

        return nullptr;
    }

    TeamSimState* opponentTeam(
        MatchSimulationState& state,
        TeamId teamId) {
        if (state.homeTeam.teamId == teamId) {
            return &state.awayTeam;
        }
        if (state.awayTeam.teamId == teamId) {
            return &state.homeTeam;
        }

        return nullptr;
    }

    PlayerSimState* findPlayerInTeam(TeamSimState& team, PlayerId playerId) {
        for (PlayerSimState& player : team.players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }

        return nullptr;
    }

    const PlayerSimState* findPlayerInTeam(const TeamSimState& team, PlayerId playerId) {
        for (const PlayerSimState& player : team.players) {
            if (player.playerId == playerId) {
                return &player;
            }
        }

        return nullptr;
    }

    void clearBallFlags(MatchSimulationState& state) {
        for (PlayerSimState& player : state.homeTeam.players) {
            player.hasBall = false;
        }
        for (PlayerSimState& player : state.awayTeam.players) {
            player.hasBall = false;
        }
    }

    MatchPlayerSimulationStats& playerStatsFor(
        MatchEngineResult& result,
        PlayerId playerId,
        TeamId teamId) {
        for (MatchPlayerSimulationStats& stats : result.playerStats) {
            if (stats.playerId == playerId) {
                return stats;
            }
        }

        MatchPlayerSimulationStats stats;
        stats.playerId = playerId;
        stats.teamId = teamId;
        stats.rating = 6.0;
        result.playerStats.push_back(stats);
        return result.playerStats.back();
    }

    MatchTeamSimulationStats& teamStatsFor(MatchEngineResult& result, TeamId teamId) {
        return result.homeStats.teamId == teamId ? result.homeStats : result.awayStats;
    }

    std::vector<PlayerMarkerSnapshot> buildMarkers(const MatchSimulationState& state) {
        std::vector<PlayerMarkerSnapshot> markers;
        markers.reserve(state.homeTeam.players.size() + state.awayTeam.players.size());

        for (const PlayerSimState& player : state.homeTeam.players) {
            markers.push_back(PlayerMarkerSnapshot{
                player.playerId,
                player.teamId,
                player.position,
                player.hasBall
            });
        }
        for (const PlayerSimState& player : state.awayTeam.players) {
            markers.push_back(PlayerMarkerSnapshot{
                player.playerId,
                player.teamId,
                player.position,
                player.hasBall
            });
        }

        return markers;
    }

    void appendTrace(
        MatchEngineResult& result,
        MatchSimulationDetail detail,
        const MatchSimulationState& state,
        MatchTraceKind kind,
        TeamId teamId,
        PlayerId primaryPlayerId,
        PlayerId secondaryPlayerId,
        PitchPoint ballPosition,
        PitchPoint ballTarget) {
        if (!shouldTrace(detail)) {
            return;
        }

        MatchTraceFrame frame;
        frame.second = state.currentSecond;
        frame.kind = kind;
        frame.teamId = teamId;
        frame.primaryPlayerId = primaryPlayerId;
        frame.secondaryPlayerId = secondaryPlayerId;
        frame.ballPosition = PitchGeometry::clampToPitch(ballPosition);
        frame.ballTarget = PitchGeometry::clampToPitch(ballTarget);

        if (detail == MatchSimulationDetail::WatchedMatch
            || detail == MatchSimulationDetail::DebugFullTrace) {
            frame.markers = buildMarkers(state);
        }

        result.traceFrames.push_back(frame);
    }

    std::vector<PlayerShapeTarget> buildTargetsSafely(
        const TeamShapeModel& shapeModel,
        const TeamShapeContext& context,
        const TeamSheet& teamSheet) {
        try {
            return shapeModel.buildTargets(context, teamSheet);
        }
        catch (const std::exception&) {
            return {};
        }
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

    TeamShapePhase shapePhaseFor(const MatchSimulationState& state, TeamId teamId) {
        if (state.ball.controlState == BallControlState::Loose) {
            return TeamShapePhase::DefensiveTransition;
        }

        if (state.possession.teamInPossession == teamId) {
            return state.possession.isTransition
                ? TeamShapePhase::AttackingTransition
                : TeamShapePhase::InPossession;
        }

        return state.possession.isTransition
            ? TeamShapePhase::DefensiveTransition
            : TeamShapePhase::OutOfPossession;
    }

    TeamShapeContext shapeContextFor(
        const TeamSimState& team,
        const MatchSimulationState& state) {
        TeamShapeContext context;
        context.teamId = team.teamId;
        context.isHomeTeam = team.side == TeamSide::Home;
        context.hasPossession = state.possession.teamInPossession == team.teamId;
        context.phase = shapePhaseFor(state, team.teamId);
        context.attackingDirection = attackingDirectionFor(context.isHomeTeam);
        context.tacticalSetup = team.tacticalSetup;
        context.ballPosition = state.ball.position;
        return context;
    }

    PlayerIntent defaultIntentFor(bool inPossession, PitchPoint target) {
        PlayerIntent intent;
        intent.type = inPossession
            ? PlayerIntentType::HoldAttackingShape
            : PlayerIntentType::HoldDefensiveShape;
        intent.target = PitchGeometry::clampToPitch(target);
        intent.urgency = 0.0;
        return intent;
    }

    std::vector<PlayerId> starterOrder(const TeamSheet& teamSheet) {
        std::vector<PlayerId> starters;
        if (!teamSheet.startingAssignments.empty()) {
            starters.reserve(teamSheet.startingAssignments.size());
            for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
                if (assignment.playerId != 0) {
                    starters.push_back(assignment.playerId);
                }
            }
            return starters;
        }

        return teamSheet.startingPlayerIds;
    }

    TeamSimState buildTeamState(
        const MatchTeamSnapshot& snapshot,
        TeamSide side,
        const std::vector<PlayerShapeTarget>& targets,
        bool startsInPossession) {
        TeamSimState team;
        team.leagueId = snapshot.leagueId;
        team.teamId = snapshot.teamId;
        team.side = side;
        team.tacticalSetup = snapshot.tacticalSetup;

        const std::vector<PlayerId> starters = starterOrder(snapshot.teamSheet);
        team.players.reserve(starters.size());

        for (PlayerId playerId : starters) {
            const MatchPlayerSnapshot* playerSnapshot = findSnapshot(snapshot, playerId);
            const PlayerShapeTarget* target = shapeTargetFor(targets, playerId);
            if (playerSnapshot == nullptr || target == nullptr) {
                continue;
            }

            const int fitness = playerSnapshot->conditionState.getFitness();
            const double fatigue = 1.0 - (std::clamp(fitness, 0, 100) / 100.0);
            const PitchPoint initialPosition = PitchGeometry::clampToPitch(target->finalTarget);

            PlayerSimState player;
            player.playerId = playerSnapshot->playerId;
            player.teamId = snapshot.teamId;
            player.side = side;
            player.position = initialPosition;
            player.targetPosition = initialPosition;
            player.currentIntent = defaultIntentFor(startsInPossession, initialPosition);
            player.fatigue = fatigue;
            player.effectivePace = 3.0 + (playerSnapshot->attributes.physical.pace / 20.0);
            player.effectiveAcceleration =
                3.0 + (playerSnapshot->attributes.physical.acceleration / 20.0);
            player.baseOverall = playerSnapshot->baseOverall;
            team.players.push_back(player);
        }

        return team;
    }

    FormationSlotRole assignedRoleFor(const TeamSheet& teamSheet, PlayerId playerId) {
        for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }

        return FormationSlotRole::Unknown;
    }

    int kickoffPriority(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Striker:
            return 0;
        case FormationSlotRole::AttackingMidfielder:
            return 1;
        case FormationSlotRole::CentralMidfielder:
            return 2;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            return 3;
        }

        return 3;
    }

    PlayerId chooseKickoffCarrier(
        const MatchTeamSnapshot& homeSnapshot,
        const TeamSimState& homeTeam) {
        PlayerId selected = 0;
        int bestPriority = std::numeric_limits<int>::max();

        const std::vector<PlayerId> starters = starterOrder(homeSnapshot.teamSheet);
        for (PlayerId playerId : starters) {
            if (findPlayerInTeam(homeTeam, playerId) == nullptr) {
                continue;
            }

            const int priority = kickoffPriority(assignedRoleFor(homeSnapshot.teamSheet, playerId));
            if (selected == 0 || priority < bestPriority) {
                selected = playerId;
                bestPriority = priority;
            }
        }

        if (selected != 0) {
            return selected;
        }

        return homeTeam.players.empty() ? 0 : homeTeam.players.front().playerId;
    }

    MatchSimulationState initializeState(const MatchEngineInput& input) {
        MatchSimulationState state;
        state.matchId = input.matchId;
        state.leagueId = input.leagueId;
        state.phase = MatchPhase::FirstHalf;
        state.currentSecond = 0;
        state.ball.position = pitchCenter();

        TeamShapeModel shapeModel;
        MatchSimulationState shapeSeedState;
        shapeSeedState.possession.teamInPossession = input.homeTeam.teamId;
        shapeSeedState.ball.position = pitchCenter();

        TeamSimState homeSeed;
        homeSeed.teamId = input.homeTeam.teamId;
        homeSeed.side = TeamSide::Home;
        homeSeed.tacticalSetup = input.homeTeam.tacticalSetup;

        TeamSimState awaySeed;
        awaySeed.teamId = input.awayTeam.teamId;
        awaySeed.side = TeamSide::Away;
        awaySeed.tacticalSetup = input.awayTeam.tacticalSetup;

        const std::vector<PlayerShapeTarget> homeTargets = buildTargetsSafely(
            shapeModel,
            shapeContextFor(homeSeed, shapeSeedState),
            input.homeTeam.teamSheet);
        const std::vector<PlayerShapeTarget> awayTargets = buildTargetsSafely(
            shapeModel,
            shapeContextFor(awaySeed, shapeSeedState),
            input.awayTeam.teamSheet);

        state.homeTeam = buildTeamState(input.homeTeam, TeamSide::Home, homeTargets, true);
        state.awayTeam = buildTeamState(input.awayTeam, TeamSide::Away, awayTargets, false);

        const PlayerId kickoffPlayerId = chooseKickoffCarrier(input.homeTeam, state.homeTeam);
        PlayerSimState* kickoffPlayer = findPlayerState(state, kickoffPlayerId);

        state.ball.controlState = BallControlState::Controlled;
        state.ball.carrierTeamId = input.homeTeam.teamId;
        state.ball.carrierPlayerId = kickoffPlayerId;
        state.ball.position = kickoffPlayer != nullptr ? kickoffPlayer->position : pitchCenter();
        state.ball.trajectory = std::nullopt;

        if (kickoffPlayer != nullptr) {
            kickoffPlayer->hasBall = true;
        }

        state.possession.teamInPossession = input.homeTeam.teamId;
        state.possession.ballCarrierId = kickoffPlayerId;
        state.possession.phase = PossessionPhase::BuildUp;
        state.possession.possessionStartSecond = 0;
        state.possession.actionDepth = 0;
        state.possession.isTransition = false;

        return state;
    }

    IntentTeamMode teamModeFor(
        const MatchSimulationState& state,
        TeamId teamId) {
        if (state.ball.controlState == BallControlState::Loose) {
            return IntentTeamMode::NeutralBall;
        }
        if (state.ball.controlState == BallControlState::InFlight) {
            return state.possession.teamInPossession == teamId
                ? IntentTeamMode::Attacking
                : IntentTeamMode::Defending;
        }
        if (state.ball.controlState == BallControlState::Controlled) {
            return state.ball.carrierTeamId == teamId
                ? IntentTeamMode::Attacking
                : IntentTeamMode::Defending;
        }

        return IntentTeamMode::NeutralBall;
    }

    std::vector<ResolvedPlayerIntent> resolveTeamIntents(
        const PlayerIntentResolver& resolver,
        const TeamSimState& team,
        const TeamSimState& opponent,
        const TeamShapeContext& shapeContext,
        const std::vector<PlayerShapeTarget>& shapeTargets,
        const std::optional<ContestResolverResult>& lastContestResult,
        const MatchSimulationState& state,
        const TeamSheet& teamSheet,
        const TeamSheet& opponentSheet,
        std::uint64_t seed) {
        PlayerIntentResolutionContext context;
        context.teamId = team.teamId;
        context.teamMode = teamModeFor(state, team.teamId);
        context.attackingDirection = shapeContext.attackingDirection;
        context.tacticalSetup = team.tacticalSetup;
        context.ballState = state.ball;
        context.lastContestResult = lastContestResult;
        context.shapeTargets = shapeTargets;
        context.teammates = team.players;
        context.opponents = opponent.players;
        context.teamAssignments = teamSheet.startingAssignments;
        context.opponentAssignments = opponentSheet.startingAssignments;
        context.seed = seed;
        return resolver.resolveTeamIntents(context);
    }

    const ResolvedPlayerIntent* resolvedIntentFor(
        const std::vector<ResolvedPlayerIntent>& intents,
        PlayerId playerId) {
        for (const ResolvedPlayerIntent& intent : intents) {
            if (intent.playerId == playerId) {
                return &intent;
            }
        }

        return nullptr;
    }

    void moveTeamPlayers(
        const MovementResolver& resolver,
        TeamSimState& team,
        const std::vector<ResolvedPlayerIntent>& intents,
        double deltaSeconds) {
        for (PlayerSimState& player : team.players) {
            const ResolvedPlayerIntent* resolved = resolvedIntentFor(intents, player.playerId);
            if (resolved == nullptr) {
                continue;
            }

            const MovementResolutionResult movement = resolver.resolve(MovementResolutionRequest{
                player,
                resolved->intent,
                resolved->finalTarget,
                deltaSeconds
            });
            player.position = movement.newPosition;
            player.targetPosition = movement.targetPosition;
            player.currentIntent = resolved->intent;
        }
    }

    double executionQualityFor(
        const MatchPlayerSnapshot* player,
        BallCarrierActionType actionType) {
        if (player == nullptr) {
            return 60.0;
        }

        double technical = 60.0;
        switch (actionType) {
        case BallCarrierActionType::ShortPass:
        case BallCarrierActionType::BackPass:
        case BallCarrierActionType::ThroughBall:
        case BallCarrierActionType::SwitchPlay:
            technical = static_cast<double>(player->attributes.technical.passing);
            break;
        case BallCarrierActionType::LowCross:
        case BallCarrierActionType::HighCross:
        case BallCarrierActionType::Cutback:
            technical = static_cast<double>(player->attributes.technical.crossing);
            break;
        case BallCarrierActionType::Shoot:
            technical = static_cast<double>(player->attributes.technical.shooting);
            break;
        case BallCarrierActionType::Clear:
            technical = std::max(
                static_cast<double>(player->attributes.technical.passing),
                static_cast<double>(player->attributes.physical.strength));
            break;
        case BallCarrierActionType::Carry:
        case BallCarrierActionType::Dribble:
        case BallCarrierActionType::CutInside:
        case BallCarrierActionType::Hold:
            technical = static_cast<double>(player->attributes.technical.dribbling);
            break;
        }

        return clampDouble((technical * 0.75) + (player->baseOverall * 0.25), 35.0, 95.0);
    }

    bool isPassLike(BallCarrierActionType type) {
        switch (type) {
        case BallCarrierActionType::ShortPass:
        case BallCarrierActionType::BackPass:
        case BallCarrierActionType::ThroughBall:
        case BallCarrierActionType::SwitchPlay:
        case BallCarrierActionType::LowCross:
        case BallCarrierActionType::HighCross:
        case BallCarrierActionType::Cutback:
            return true;
        case BallCarrierActionType::Carry:
        case BallCarrierActionType::Dribble:
        case BallCarrierActionType::CutInside:
        case BallCarrierActionType::Shoot:
        case BallCarrierActionType::Hold:
        case BallCarrierActionType::Clear:
            return false;
        }

        return false;
    }

    BallTrajectoryType trajectoryTypeFor(BallCarrierActionType type) {
        switch (type) {
        case BallCarrierActionType::ThroughBall:
            return BallTrajectoryType::ThroughBall;
        case BallCarrierActionType::LowCross:
            return BallTrajectoryType::LowCross;
        case BallCarrierActionType::HighCross:
            return BallTrajectoryType::HighCross;
        case BallCarrierActionType::Cutback:
            return BallTrajectoryType::Cutback;
        case BallCarrierActionType::Shoot:
            return BallTrajectoryType::Shot;
        case BallCarrierActionType::Clear:
            return BallTrajectoryType::Clearance;
        case BallCarrierActionType::ShortPass:
        case BallCarrierActionType::BackPass:
        case BallCarrierActionType::SwitchPlay:
        case BallCarrierActionType::Carry:
        case BallCarrierActionType::Dribble:
        case BallCarrierActionType::CutInside:
        case BallCarrierActionType::Hold:
            return BallTrajectoryType::GroundPass;
        }

        return BallTrajectoryType::GroundPass;
    }

    MatchTraceKind traceKindFor(BallCarrierActionType type) {
        switch (type) {
        case BallCarrierActionType::ThroughBall:
            return MatchTraceKind::ThroughBall;
        case BallCarrierActionType::LowCross:
            return MatchTraceKind::LowCross;
        case BallCarrierActionType::HighCross:
            return MatchTraceKind::HighCross;
        case BallCarrierActionType::Cutback:
            return MatchTraceKind::Cutback;
        case BallCarrierActionType::Carry:
            return MatchTraceKind::Carry;
        case BallCarrierActionType::Dribble:
        case BallCarrierActionType::CutInside:
            return MatchTraceKind::Dribble;
        case BallCarrierActionType::Shoot:
            return MatchTraceKind::Shot;
        case BallCarrierActionType::Clear:
            return MatchTraceKind::Clearance;
        case BallCarrierActionType::ShortPass:
        case BallCarrierActionType::BackPass:
        case BallCarrierActionType::SwitchPlay:
        case BallCarrierActionType::Hold:
            return MatchTraceKind::Pass;
        }

        return MatchTraceKind::Pass;
    }

    void moveControlledBall(
        MatchSimulationState& state,
        PlayerSimState& carrier,
        PitchPoint target,
        double meters) {
        const PitchPoint start = carrier.position;
        const double distance = PitchGeometry::distance(start, target);
        if (distance <= 0.001) {
            state.ball.position = start;
            return;
        }

        const double progress = std::min(1.0, meters / distance);
        const PitchPoint next = PitchGeometry::clampToPitch(PitchPoint{
            start.x + ((target.x - start.x) * progress),
            start.y + ((target.y - start.y) * progress)
        });
        carrier.position = next;
        carrier.targetPosition = PitchGeometry::clampToPitch(target);
        state.ball.position = next;
    }

    void setControlledBy(
        MatchSimulationState& state,
        PlayerId playerId,
        TeamId teamId,
        PitchPoint position) {
        clearBallFlags(state);

        PlayerSimState* controller = findPlayerState(state, playerId);
        if (controller != nullptr) {
            controller->hasBall = true;
            controller->position = PitchGeometry::clampToPitch(position);
            controller->targetPosition = controller->position;
        }

        state.ball.controlState = BallControlState::Controlled;
        state.ball.carrierPlayerId = playerId;
        state.ball.carrierTeamId = teamId;
        state.ball.position = PitchGeometry::clampToPitch(position);
        state.ball.trajectory = std::nullopt;
        state.possession.teamInPossession = teamId;
        state.possession.ballCarrierId = playerId;
        state.possession.possessionStartSecond = state.currentSecond;
        state.possession.isTransition = true;
    }

    void setLooseBall(MatchSimulationState& state, PitchPoint position) {
        clearBallFlags(state);
        state.ball.controlState = BallControlState::Loose;
        state.ball.carrierPlayerId = 0;
        state.ball.carrierTeamId = 0;
        state.ball.position = PitchGeometry::clampToPitch(position);
        state.ball.trajectory = std::nullopt;
        state.possession.ballCarrierId = 0;
        state.possession.isTransition = true;
    }

    void executeControlledAction(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const TeamShapeContext& carrierShapeContext,
        const BallTrajectoryBuilder& trajectoryBuilder,
        const ActionCandidateGenerator& generator,
        const ActionSelector& selector,
        std::optional<PendingBallAction>& pending,
        std::uint64_t baseSeed,
        double deltaSeconds) {
        PlayerSimState* carrier = findPlayerState(state, state.ball.carrierPlayerId);
        if (carrier == nullptr) {
            setLooseBall(state, state.ball.position);
            return;
        }

        state.ball.position = carrier->position;
        const MatchTeamSnapshot* carrierSnapshot = snapshotForTeam(input, carrier->teamId);
        const MatchPlayerSnapshot* playerSnapshot =
            findSnapshotForPlayer(input, carrier->playerId);
        const TeamSimState* carrierTeamState = findTeamState(state, carrier->teamId);
        const TeamSimState* opponentState = opponentTeam(state, carrier->teamId);
        if (carrierSnapshot == nullptr || carrierTeamState == nullptr || opponentState == nullptr) {
            return;
        }

        const std::vector<ActionCandidate> candidates = generator.generate(
            ActionCandidateGenerationRequest{
                *carrier,
                state.ball,
                state,
                carrierShapeContext,
                carrierTeamState->players,
                opponentState->players
            });
        const ActionSelectionResult selection = selector.select(ActionSelectionRequest{
            candidates,
            playerSnapshot != nullptr ? playerSnapshot->attributes : PlayerAttributes{},
            stepSeed(baseSeed, state, static_cast<std::uint64_t>(carrier->playerId))
        });

        if (!selection.selected) {
            appendTrace(
                result,
                input.options.detail,
                state,
                MatchTraceKind::PossessionStart,
                carrier->teamId,
                carrier->playerId,
                0,
                state.ball.position,
                state.ball.position);
            return;
        }

        const ActionCandidate action = *selection.selected;
        const BallCarrierActionType actionType = action.type;
        if (actionType == BallCarrierActionType::Hold) {
            appendTrace(
                result,
                input.options.detail,
                state,
                MatchTraceKind::PossessionStart,
                carrier->teamId,
                carrier->playerId,
                0,
                state.ball.position,
                state.ball.position);
            return;
        }

        if (actionType == BallCarrierActionType::Carry
            || actionType == BallCarrierActionType::Dribble
            || actionType == BallCarrierActionType::CutInside) {
            moveControlledBall(
                state,
                *carrier,
                action.intendedTarget,
                (actionType == BallCarrierActionType::Carry ? 5.5 : 4.0) * deltaSeconds);
            appendTrace(
                result,
                input.options.detail,
                state,
                traceKindFor(actionType),
                carrier->teamId,
                carrier->playerId,
                0,
                state.ball.position,
                action.intendedTarget);
            return;
        }

        const BallTrajectoryType trajectoryType = trajectoryTypeFor(actionType);
        const BallTrajectoryBuildResult trajectory = trajectoryBuilder.build(BallTrajectoryBuildRequest{
            state.ball.position,
            action.intendedTarget,
            trajectoryType,
            static_cast<double>(state.currentSecond),
            executionQualityFor(playerSnapshot, actionType),
            action.pressurePenalty,
            stepSeed(
                baseSeed,
                state,
                static_cast<std::uint64_t>(carrier->playerId)
                    ^ (static_cast<std::uint64_t>(actionType) << 32))
        });

        if (isPassLike(actionType)) {
            ++teamStatsFor(result, carrier->teamId).passesAttempted;
            ++playerStatsFor(result, carrier->playerId, carrier->teamId).passesAttempted;
        } else if (actionType == BallCarrierActionType::Shoot) {
            ++teamStatsFor(result, carrier->teamId).shots;
            ++playerStatsFor(result, carrier->playerId, carrier->teamId).shots;
        }

        clearBallFlags(state);
        state.ball.controlState = BallControlState::InFlight;
        state.ball.carrierPlayerId = 0;
        state.ball.carrierTeamId = 0;
        state.ball.position = trajectory.trajectory.start;
        state.ball.trajectory = trajectory.trajectory;
        state.possession.teamInPossession = carrier->teamId;
        state.possession.ballCarrierId = 0;

        pending = PendingBallAction{
            carrier->teamId,
            carrier->playerId,
            action.targetPlayerId,
            actionType,
            trajectoryType,
            actionType == BallCarrierActionType::Shoot
        };

        appendTrace(
            result,
            input.options.detail,
            state,
            traceKindFor(actionType),
            carrier->teamId,
            carrier->playerId,
            action.targetPlayerId,
            trajectory.trajectory.start,
            trajectory.trajectory.actualTarget);
    }

    ContestType contestTypeFor(const PendingBallAction& pending) {
        if (pending.isShot) {
            return ContestType::ShotBlock;
        }
        if (pending.trajectoryType == BallTrajectoryType::LowCross
            || pending.trajectoryType == BallTrajectoryType::HighCross
            || pending.trajectoryType == BallTrajectoryType::Cutback) {
            return ContestType::GroundCrossInterception;
        }

        return ContestType::PassingLaneInterception;
    }

    ContestParticipant participantFor(
        const PlayerSimState& player,
        const MatchPlayerSnapshot* snapshot,
        ContestSide side,
        double arrivalSecond,
        double startingAdvantage) {
        ContestParticipant participant;
        participant.playerId = player.playerId;
        participant.teamId = player.teamId;
        participant.side = side;
        participant.position = player.position;
        participant.attributes = snapshot != nullptr ? snapshot->attributes : PlayerAttributes{};
        participant.baseOverall = snapshot != nullptr ? snapshot->baseOverall : player.baseOverall;
        participant.fatigue = player.fatigue;
        participant.effectivePace = player.effectivePace;
        participant.effectiveAcceleration = player.effectiveAcceleration;
        participant.arrivalSecond = arrivalSecond;
        participant.startingAdvantage = startingAdvantage;
        return participant;
    }

    void processInFlightBall(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const BallTrajectoryBuilder& trajectoryBuilder,
        const InterceptionResolver& interceptionResolver,
        const ContestResolver& contestResolver,
        std::optional<ContestResolverResult>& lastContestResult,
        std::optional<PendingBallAction>& pending,
        std::uint64_t baseSeed) {
        if (!state.ball.trajectory) {
            setLooseBall(state, state.ball.position);
            pending = std::nullopt;
            return;
        }

        const BallTrajectory trajectory = *state.ball.trajectory;
        TeamSimState* defendingTeam = opponentTeam(state, state.possession.teamInPossession);
        if (defendingTeam == nullptr) {
            setLooseBall(state, trajectory.actualTarget);
            pending = std::nullopt;
            return;
        }

        const InterceptionResolverResult interception = interceptionResolver.resolve(
            InterceptionResolverRequest{
                trajectory,
                defendingTeam->players,
                7,
                0.35
            });

        if (pending && interception.bestCandidate && interception.bestCandidate->qualityScore >= 6.0) {
            const InterceptionCandidate candidate = *interception.bestCandidate;
            const PlayerSimState* defender = findPlayerState(state, candidate.playerId);
            const PlayerSimState* attacker =
                pending->targetPlayerId != 0
                    ? findPlayerState(state, pending->targetPlayerId)
                    : findPlayerState(state, pending->sourcePlayerId);

            if (defender != nullptr && attacker != nullptr) {
                ContestResolverRequest request;
                request.type = contestTypeFor(*pending);
                request.contestPoint = candidate.interceptionPoint;
                request.interceptionCandidate = candidate;
                request.ballArrivalSecond = candidate.ballArrivalSecond;
                request.pressure = 35.0;
                request.executionQuality = 70.0;
                request.seed = stepSeed(
                    baseSeed,
                    state,
                    static_cast<std::uint64_t>(candidate.playerId)
                        ^ (static_cast<std::uint64_t>(pending->sourcePlayerId) << 32)
                        ^ static_cast<std::uint64_t>(request.type));
                request.participants.push_back(participantFor(
                    *defender,
                    findSnapshotForPlayer(input, defender->playerId),
                    ContestSide::Defending,
                    candidate.playerArrivalSecond,
                    candidate.qualityScore));
                request.participants.push_back(participantFor(
                    *attacker,
                    findSnapshotForPlayer(input, attacker->playerId),
                    ContestSide::Attacking,
                    trajectory.arrivalSecond,
                    0.0));

                const ContestResolverResult contest = contestResolver.resolve(request);
                lastContestResult = contest;

                if (contest.cleanController) {
                    const TeamId previousTeam = state.possession.teamInPossession;
                    setControlledBy(
                        state,
                        contest.cleanController->playerId,
                        contest.cleanController->teamId,
                        candidate.interceptionPoint);

                    if (contest.cleanController->teamId != previousTeam) {
                        ++teamStatsFor(result, contest.cleanController->teamId).interceptions;
                        ++playerStatsFor(
                            result,
                            contest.cleanController->playerId,
                            contest.cleanController->teamId).interceptions;
                        appendTrace(
                            result,
                            input.options.detail,
                            state,
                            MatchTraceKind::Turnover,
                            contest.cleanController->teamId,
                            contest.cleanController->playerId,
                            pending->sourcePlayerId,
                            candidate.interceptionPoint,
                            candidate.interceptionPoint);
                    }

                    appendTrace(
                        result,
                        input.options.detail,
                        state,
                        MatchTraceKind::Interception,
                        contest.cleanController->teamId,
                        contest.cleanController->playerId,
                        pending->sourcePlayerId,
                        candidate.interceptionPoint,
                        candidate.interceptionPoint);
                    pending = std::nullopt;
                    return;
                }

                if (contest.ballDeflected) {
                    state.ball.controlState = BallControlState::InFlight;
                    state.ball.position = candidate.interceptionPoint;
                    state.ball.trajectory = trajectoryBuilder.buildDeflectedTrajectory(
                        DeflectedBallTrajectoryRequest{
                            candidate.interceptionPoint,
                            trajectory.start,
                            trajectory.actualTarget,
                            0.55,
                            candidate.ballArrivalSecond,
                            stepSeed(baseSeed, state, candidate.playerId ^ 0xdef1ec7ULL)
                        });
                    appendTrace(
                        result,
                        input.options.detail,
                        state,
                        MatchTraceKind::Interception,
                        candidate.teamId,
                        candidate.playerId,
                        pending->sourcePlayerId,
                        candidate.interceptionPoint,
                        state.ball.trajectory->actualTarget);
                    pending = PendingBallAction{
                        pending->sourceTeamId,
                        pending->sourcePlayerId,
                        0,
                        pending->actionType,
                        BallTrajectoryType::Deflection,
                        pending->isShot
                    };
                    return;
                }

                if (contest.ballBecomesLoose
                    || contest.ballOutcome == ContestBallOutcome::BallLoose) {
                    setLooseBall(state, candidate.interceptionPoint);
                    appendTrace(
                        result,
                        input.options.detail,
                        state,
                        MatchTraceKind::LooseBall,
                        candidate.teamId,
                        candidate.playerId,
                        pending->sourcePlayerId,
                        candidate.interceptionPoint,
                        candidate.interceptionPoint);
                    pending = std::nullopt;
                    return;
                }

                if (contest.ballOutcome == ContestBallOutcome::ShotContinues) {
                    setLooseBall(state, trajectory.actualTarget);
                    appendTrace(
                        result,
                        input.options.detail,
                        state,
                        MatchTraceKind::LooseBall,
                        pending->sourceTeamId,
                        pending->sourcePlayerId,
                        0,
                        trajectory.actualTarget,
                        trajectory.actualTarget);
                    pending = std::nullopt;
                    return;
                }
            }
        }

        state.ball.position = PitchGeometry::clampToPitch(trajectory.actualTarget);
        if (pending
            && !pending->isShot
            && pending->targetPlayerId != 0) {
            PlayerSimState* target = findPlayerState(state, pending->targetPlayerId);
            if (target != nullptr
                && PitchGeometry::distance(target->position, state.ball.position)
                    <= ReceptionControlRangeMeters) {
                setControlledBy(
                    state,
                    target->playerId,
                    target->teamId,
                    state.ball.position);
                if (target->teamId == pending->sourceTeamId && isPassLike(pending->actionType)) {
                    ++teamStatsFor(result, pending->sourceTeamId).passesCompleted;
                    ++playerStatsFor(
                        result,
                        pending->sourcePlayerId,
                        pending->sourceTeamId).passesCompleted;
                }
                pending = std::nullopt;
                return;
            }
        }

        setLooseBall(state, state.ball.position);
        appendTrace(
            result,
            input.options.detail,
            state,
            MatchTraceKind::LooseBall,
            pending ? pending->sourceTeamId : state.possession.teamInPossession,
            pending ? pending->sourcePlayerId : 0,
            0,
            state.ball.position,
            state.ball.position);
        pending = std::nullopt;
    }

    void processLooseBall(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input) {
        PlayerSimState* best = nullptr;
        double bestDistance = std::numeric_limits<double>::max();

        for (PlayerSimState& player : state.homeTeam.players) {
            const double distance = PitchGeometry::distance(player.position, state.ball.position);
            if (distance < bestDistance) {
                best = &player;
                bestDistance = distance;
            }
        }
        for (PlayerSimState& player : state.awayTeam.players) {
            const double distance = PitchGeometry::distance(player.position, state.ball.position);
            if (distance < bestDistance) {
                best = &player;
                bestDistance = distance;
            }
        }

        if (best == nullptr || bestDistance > LooseBallControlRangeMeters) {
            return;
        }

        const TeamId previousTeam = state.possession.teamInPossession;
        setControlledBy(state, best->playerId, best->teamId, state.ball.position);
        appendTrace(
            result,
            input.options.detail,
            state,
            previousTeam != 0 && previousTeam != best->teamId
                ? MatchTraceKind::Turnover
                : MatchTraceKind::PossessionStart,
            best->teamId,
            best->playerId,
            0,
            state.ball.position,
            state.ball.position);
    }

    void processOutOfPlay(MatchSimulationState& state) {
        setLooseBall(state, pitchCenter());
    }

    void addInitialPlayerStats(MatchEngineResult& result, const MatchSimulationState& state) {
        for (const PlayerSimState& player : state.homeTeam.players) {
            playerStatsFor(result, player.playerId, player.teamId);
        }
        for (const PlayerSimState& player : state.awayTeam.players) {
            playerStatsFor(result, player.playerId, player.teamId);
        }
    }

    void finalizePossessionShare(MatchEngineResult& result, const MatchSimulationState& state) {
        const double total =
            state.homeTeam.possessionShareAccumulator
            + state.awayTeam.possessionShareAccumulator;
        if (total <= 0.001) {
            result.homeStats.possessionShare = 50.0;
            result.awayStats.possessionShare = 50.0;
            return;
        }

        result.homeStats.possessionShare =
            (state.homeTeam.possessionShareAccumulator / total) * 100.0;
        result.awayStats.possessionShare =
            (state.awayTeam.possessionShareAccumulator / total) * 100.0;
    }
}

MatchEngineResult CoordinateSimulationPrototype::run(const MatchEngineInput& input) const {
    MatchEngineResult result;
    result.homeStats.teamId = input.homeTeam.teamId;
    result.awayStats.teamId = input.awayTeam.teamId;

    MatchSimulationState state = initializeState(input);
    addInitialPlayerStats(result, state);

    const std::uint64_t baseSeed = baseSeedFor(input);
    const double deltaSeconds = deltaSecondsFor(input);
    const int maxActions = maxActionsFor(input.options.detail);

    TeamShapeModel shapeModel;
    PlayerIntentResolver intentResolver;
    MovementResolver movementResolver;
    ActionCandidateGenerator actionGenerator;
    ActionSelector actionSelector;
    BallTrajectoryBuilder trajectoryBuilder;
    InterceptionResolver interceptionResolver;
    ContestResolver contestResolver;

    std::optional<ContestResolverResult> lastContestResult;
    std::optional<PendingBallAction> pending;

    appendTrace(
        result,
        input.options.detail,
        state,
        MatchTraceKind::PossessionStart,
        state.ball.carrierTeamId,
        state.ball.carrierPlayerId,
        0,
        state.ball.position,
        state.ball.position);

    for (int actionIndex = 0; actionIndex < maxActions; ++actionIndex) {
        const TeamShapeContext homeShapeContext = shapeContextFor(state.homeTeam, state);
        const TeamShapeContext awayShapeContext = shapeContextFor(state.awayTeam, state);
        const std::vector<PlayerShapeTarget> homeTargets = buildTargetsSafely(
            shapeModel,
            homeShapeContext,
            input.homeTeam.teamSheet);
        const std::vector<PlayerShapeTarget> awayTargets = buildTargetsSafely(
            shapeModel,
            awayShapeContext,
            input.awayTeam.teamSheet);

        const std::vector<ResolvedPlayerIntent> homeIntents = resolveTeamIntents(
            intentResolver,
            state.homeTeam,
            state.awayTeam,
            homeShapeContext,
            homeTargets,
            lastContestResult,
            state,
            input.homeTeam.teamSheet,
            input.awayTeam.teamSheet,
            stepSeed(baseSeed, state, 0x101ULL));
        const std::vector<ResolvedPlayerIntent> awayIntents = resolveTeamIntents(
            intentResolver,
            state.awayTeam,
            state.homeTeam,
            awayShapeContext,
            awayTargets,
            lastContestResult,
            state,
            input.awayTeam.teamSheet,
            input.homeTeam.teamSheet,
            stepSeed(baseSeed, state, 0x202ULL));

        moveTeamPlayers(movementResolver, state.homeTeam, homeIntents, deltaSeconds);
        moveTeamPlayers(movementResolver, state.awayTeam, awayIntents, deltaSeconds);

        if (state.possession.teamInPossession == state.homeTeam.teamId) {
            state.homeTeam.possessionShareAccumulator += deltaSeconds;
        } else if (state.possession.teamInPossession == state.awayTeam.teamId) {
            state.awayTeam.possessionShareAccumulator += deltaSeconds;
        }

        if (state.ball.controlState == BallControlState::Controlled) {
            const TeamShapeContext& carrierContext =
                state.ball.carrierTeamId == state.homeTeam.teamId
                    ? homeShapeContext
                    : awayShapeContext;
            executeControlledAction(
                state,
                result,
                input,
                carrierContext,
                trajectoryBuilder,
                actionGenerator,
                actionSelector,
                pending,
                baseSeed,
                deltaSeconds);
        } else if (state.ball.controlState == BallControlState::InFlight) {
            processInFlightBall(
                state,
                result,
                input,
                trajectoryBuilder,
                interceptionResolver,
                contestResolver,
                lastContestResult,
                pending,
                baseSeed);
        } else if (state.ball.controlState == BallControlState::Loose) {
            processLooseBall(state, result, input);
        } else if (state.ball.controlState == BallControlState::OutOfPlay) {
            processOutOfPlay(state);
        }

        ++state.possession.actionDepth;
        state.currentSecond += static_cast<int>(std::ceil(deltaSeconds));
    }

    finalizePossessionShare(result, state);
    result.report = std::nullopt;
    return result;
}
