#include"fm/match_engine/simulation/CoordinateMatchSimulator.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/ActionSelector.h"
#include"fm/match_engine/decision/BallCarrierDecisionModel.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/ball/BallTrajectoryBuilder.h"
#include"fm/match_engine/ball/LooseBallRecoveryModel.h"
#include"fm/match_engine/ball/PassResolutionFlow.h"
#include"fm/match_engine/ball/ReboundTrajectoryBuilder.h"
#include"fm/match_engine/ball/ShotBlockResolver.h"
#include"fm/match_engine/ball/ShotContextBuilder.h"
#include"fm/match_engine/ball/ShotExecutionModel.h"
#include"fm/match_engine/ball/ShotOutcomeResolver.h"
#include"fm/match_engine/ball/ShotTargetSelector.h"
#include"fm/match_engine/ball/ShotTrajectoryBuilder.h"
#include"fm/match_engine/ball/ShotTypeSelector.h"
#include"fm/match_engine/contest/ContestResolver.h"
#include"fm/match_engine/contest/InterceptionResolver.h"
#include"fm/match_engine/reporting/MatchEngineReportAdapter.h"
#include"fm/match_engine/reporting/PlayerRatingModel.h"
#include"fm/match_engine/movement/MovementResolver.h"
#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/decision/PlayerIntentResolver.h"
#include"fm/match_engine/ball/ShotQualityModel.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<algorithm>
#include<cmath>
#include<cstdint>
#include<initializer_list>
#include<limits>
#include<optional>
#include<stdexcept>
#include<vector>

namespace {
    constexpr int RegulationMatchSeconds = 90 * 60;
    constexpr int MaxSafetyActions = 8000;
    constexpr double MinimumStepSeconds = 0.5;
    constexpr double MaximumNormalStepSeconds = 15.0;
    constexpr double RestartAfterGoalSeconds = 20.0;

    struct SimulationStepResult {
        double elapsedSeconds = 1.0;
        bool controlledActionExecuted = false;
    };

    enum class PendingBallKind {
        PlayerAction,
        SavedRebound,
        BlockedDeflection,
        LooseDeflection
    };

    struct PendingBallAction {
        PendingBallKind kind = PendingBallKind::PlayerAction;
        TeamId sourceTeamId = 0;
        PlayerId sourcePlayerId = 0;
        PlayerId targetPlayerId = 0;
        // Meaningful only when kind == PlayerAction.
        BallCarrierActionType actionType = BallCarrierActionType::Hold;
        BallTrajectoryType trajectoryType = BallTrajectoryType::GroundPass;
        double executionQuality = 70.0;
        double pressure = 0.0;
        bool isShot = false;
        double shotXG = 0.0;
        ShotType shotType = ShotType::ControlledFinish;
        ShotContext shotContext;
        ShotTargetSelectionResult shotTarget;
        ShotExecutionResult shotExecution;
        ShotQualityResult shotQuality;
    };

    PendingBallAction pendingPlayerAction(
        TeamId sourceTeamId,
        PlayerId sourcePlayerId,
        PlayerId targetPlayerId,
        BallCarrierActionType actionType,
        BallTrajectoryType trajectoryType,
        double executionQuality,
        double pressure,
        bool isShot,
        double shotXG,
        ShotType shotType = ShotType::ControlledFinish,
        ShotContext shotContext = ShotContext{},
        ShotTargetSelectionResult shotTarget = ShotTargetSelectionResult{},
        ShotExecutionResult shotExecution = ShotExecutionResult{},
        ShotQualityResult shotQuality = ShotQualityResult{}) {
        PendingBallAction pending;
        pending.kind = PendingBallKind::PlayerAction;
        pending.sourceTeamId = sourceTeamId;
        pending.sourcePlayerId = sourcePlayerId;
        pending.targetPlayerId = targetPlayerId;
        pending.actionType = actionType;
        pending.trajectoryType = trajectoryType;
        pending.executionQuality = executionQuality;
        pending.pressure = pressure;
        pending.isShot = isShot;
        pending.shotXG = shotXG;
        pending.shotType = shotType;
        pending.shotContext = shotContext;
        pending.shotTarget = shotTarget;
        pending.shotExecution = shotExecution;
        pending.shotQuality = shotQuality;
        return pending;
    }

    PendingBallAction pendingUncontrolledBall(
        PendingBallKind kind,
        TeamId sourceTeamId,
        PlayerId sourcePlayerId,
        BallTrajectoryType trajectoryType) {
        PendingBallAction pending;
        pending.kind = kind;
        pending.sourceTeamId = sourceTeamId;
        pending.sourcePlayerId = sourcePlayerId;
        pending.trajectoryType = trajectoryType;
        return pending;
    }

    struct AssistTracker {
        PlayerId passerPlayerId = 0;
        PlayerId receiverPlayerId = 0;
        TeamId teamId = 0;
        int second = 0;
    };

    bool isPassLike(BallCarrierActionType type);

    PitchPoint pitchCenter() {
        return PitchPoint{
            PitchGeometry::LengthMeters / 2.0,
            PitchGeometry::WidthMeters / 2.0
        };
    }

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double clampElapsedSeconds(
        double value,
        double minimum = MinimumStepSeconds,
        double maximum = MaximumNormalStepSeconds) {
        if (!std::isfinite(value) || value <= 0.0) {
            return minimum;
        }
        return clampDouble(value, minimum, maximum);
    }

    double clampedAttribute(int value) {
        return std::clamp(static_cast<double>(value), 0.0, 100.0);
    }

    double attributeAverage(std::initializer_list<double> values) {
        if (values.size() == 0) {
            return 50.0;
        }

        double total = 0.0;
        for (double value : values) {
            total += value;
        }
        return total / static_cast<double>(values.size());
    }

    void clearAssist(AssistTracker& assistTracker) {
        assistTracker = AssistTracker{};
    }

    // Temporary attribute-only reach model until player height is modeled.
    double effectiveAerialReachMeters(const PlayerAttributes& attributes, bool goalkeeper) {
        if (goalkeeper) {
            return 1.75
                + (clampedAttribute(attributes.physical.jumpingReach) / 100.0 * 0.75)
                + (clampedAttribute(attributes.goalkeeper.aerialAbility) / 100.0 * 0.45);
        }

        return 1.65
            + (clampedAttribute(attributes.physical.jumpingReach) / 100.0 * 0.85)
            + (clampedAttribute(attributes.technical.heading) / 100.0 * 0.15);
    }

    double effectiveGroundInterventionReachMeters(const PlayerAttributes& attributes) {
        return 0.8
            + (clampedAttribute(attributes.physical.agility) / 100.0 * 0.25)
            + (clampedAttribute(attributes.technical.tackling) / 100.0 * 0.25);
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

    const MatchTeamSnapshot* snapshotForPlayerTeam(
        const MatchEngineInput& input,
        TeamId teamId,
        PlayerId playerId) {
        const MatchTeamSnapshot* team = snapshotForTeam(input, teamId);
        if (team != nullptr && findSnapshot(*team, playerId) != nullptr) {
            return team;
        }

        return nullptr;
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

    bool isAssignedGoalkeeper(const MatchEngineInput& input, TeamId teamId, PlayerId playerId) {
        const MatchTeamSnapshot* team = snapshotForPlayerTeam(input, teamId, playerId);
        return team != nullptr
            && assignedRoleFor(team->teamSheet, playerId) == FormationSlotRole::Goalkeeper;
    }

    bool isInterceptionIntent(PlayerIntentType type) {
        return type == PlayerIntentType::InterceptBallPath
            || type == PlayerIntentType::BlockPassingLane
            || type == PlayerIntentType::PressBallCarrier
            || type == PlayerIntentType::ContainBallCarrier
            || type == PlayerIntentType::CoverSpace
            || type == PlayerIntentType::MarkOpponent;
    }

    double distancePointToSegment(PitchPoint point, PitchPoint start, PitchPoint end);

    bool isRoutinePass(BallCarrierActionType type) {
        return type == BallCarrierActionType::BackPass
            || type == BallCarrierActionType::ShortPass
            || type == BallCarrierActionType::SwitchPlay;
    }

    std::vector<PlayerSimState> interceptionDefendersFor(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam,
        const std::optional<PendingBallAction>& pending,
        const BallTrajectory& trajectory) {
        std::vector<PlayerSimState> defenders;
        defenders.reserve(defendingTeam.players.size());
        for (const PlayerSimState& player : defendingTeam.players) {
            if (pending && pending->isShot
                && isAssignedGoalkeeper(input, defendingTeam.teamId, player.playerId)) {
                continue;
            }

            if (pending && !pending->isShot && isRoutinePass(pending->actionType)) {
                const double distanceToStart = PitchGeometry::distance(player.position, trajectory.start);
                const double distanceToEnd = PitchGeometry::distance(player.position, trajectory.actualTarget);
                const double distanceToLane =
                    distancePointToSegment(player.position, trajectory.start, trajectory.actualTarget);
                if (!isInterceptionIntent(player.currentIntent.type)
                    && distanceToLane > 7.5
                    && std::min(distanceToStart, distanceToEnd) > 10.0) {
                    continue;
                }
            }

            defenders.push_back(player);
        }
        return defenders;
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
        state.possession.lastPossessionTeamId = input.homeTeam.teamId;
        state.possession.ballCarrierId = kickoffPlayerId;
        state.possession.phase = PossessionPhase::BuildUp;
        state.possession.possessionStartSecond = 0;
        state.possession.actionDepth = 0;
        state.possession.possessionStartPoint = state.ball.position;
        state.possession.lastMeaningfulProgressionPoint = state.ball.position;
        state.possession.lastMeaningfulProgressionSecond = 0;
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

    DefensiveContext buildDefensiveContextForTeam(
        const MatchSimulationState& state,
        const TeamSimState& defendingTeam,
        const TeamSimState& opponentTeam,
        AttackingDirection defendingDirection);

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
        context.defensiveContext = buildDefensiveContextForTeam(
            state,
            team,
            opponent,
            shapeContext.attackingDirection);
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
        double deltaSeconds,
        PlayerId skipPlayerId = 0) {
        for (PlayerSimState& player : team.players) {
            if (player.playerId == skipPlayerId) {
                continue;
            }

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
        const TeamId previousTeam = state.possession.teamInPossession != 0
            ? state.possession.teamInPossession
            : state.possession.lastPossessionTeamId;
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
        state.possession.lastPossessionTeamId = teamId;
        state.possession.ballCarrierId = playerId;
        if (previousTeam != teamId) {
            state.possession.possessionStartSecond = state.currentSecond;
            state.possession.actionDepth = 0;
            state.possession.possessionStartPoint = state.ball.position;
            state.possession.lastMeaningfulProgressionPoint = state.ball.position;
            state.possession.lastMeaningfulProgressionSecond = state.currentSecond;
            state.possession.isTransition = true;
        } else {
            state.possession.isTransition = false;
        }
    }

    double nearestOpponentPressure(PitchPoint position, const std::vector<PlayerSimState>& opponents);
    double ballProgression(PitchPoint point, AttackingDirection direction);
    AttackingDirection attackingDirectionForTeam(const TeamSimState& team);
    DecisionMatchPhase decisionPhaseFor(
        PitchPoint ballPosition,
        AttackingDirection direction,
        bool transition);
    double opponentBlockCompactness(PitchPoint ballPosition, const std::vector<PlayerSimState>& opponents);
    bool safeCirculationAvailable(
        const PlayerSimState& carrier,
        const std::vector<PlayerSimState>& teammates,
        const std::vector<PlayerSimState>& opponents);
    bool progressionLaneAvailable(
        const PlayerSimState& carrier,
        const std::vector<PlayerSimState>& teammates,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction,
        double requiredForwardMeters);
    bool entryAvailable(
        const PlayerSimState& carrier,
        const std::vector<PlayerSimState>& teammates,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction,
        double entryProgress);

    void setLooseBall(MatchSimulationState& state, PitchPoint position) {
        const TeamId lastPossessionTeamId = state.ball.carrierTeamId != 0
            ? state.ball.carrierTeamId
            : (state.possession.teamInPossession != 0
                ? state.possession.teamInPossession
                : state.possession.lastPossessionTeamId);
        clearBallFlags(state);
        state.ball.controlState = BallControlState::Loose;
        state.ball.carrierPlayerId = 0;
        state.ball.carrierTeamId = 0;
        state.ball.position = PitchGeometry::clampToPitch(position);
        state.ball.trajectory = std::nullopt;
        state.possession.teamInPossession = 0;
        state.possession.lastPossessionTeamId = lastPossessionTeamId;
        state.possession.ballCarrierId = 0;
        state.possession.isTransition = true;
    }

    struct ContextProfile {
        double safePassDistance = 20.0;
        double safeReceiverPressureLimit = 16.0;
        double forwardLaneMeters = 8.0;
        double boxEntryProgress = 0.78;
        double compactBlockRadius = 22.0;
        double pressureBlockActionCount = 8.0;
        double staleProgressionSeconds = 35.0;
        double meaningfulProgressMeters = 6.0;
    };

    PlayerDecisionContext buildPlayerDecisionContext(
        const MatchSimulationState& state,
        const TeamShapeContext& shapeContext,
        const PlayerSimState& carrier,
        FormationSlotRole role,
        const std::vector<PlayerSimState>& teammates,
        const std::vector<PlayerSimState>& opponents) {
        const ContextProfile profile;
        const double blockCompactness = opponentBlockCompactness(carrier.position, opponents);
        const double pressure = std::clamp(
            nearestOpponentPressure(carrier.position, opponents) + blockCompactness * 0.16,
            0.0,
            100.0);
        const double progress = ballProgression(carrier.position, shapeContext.attackingDirection);
        const double startProgress = ballProgression(
            state.possession.possessionStartPoint,
            shapeContext.attackingDirection);
        const double lastMeaningfulProgress = ballProgression(
            state.possession.lastMeaningfulProgressionPoint,
            shapeContext.attackingDirection);
        const double secondsSinceProgression = static_cast<double>(
            std::max(0, state.currentSecond - state.possession.lastMeaningfulProgressionSecond));
        const DecisionMatchPhase phase = decisionPhaseFor(
            carrier.position,
            shapeContext.attackingDirection,
            state.possession.isTransition);

        PossessionContext possession;
        if (state.possession.teamInPossession != 0) {
            possession.teamInPossession = state.possession.teamInPossession;
        }
        if (state.possession.ballCarrierId != 0) {
            possession.ballCarrierId = state.possession.ballCarrierId;
        }
        possession.possessionActionCount = state.possession.actionDepth;
        possession.secondsInPossession = static_cast<double>(
            std::max(0, state.currentSecond - state.possession.possessionStartSecond));
        possession.secondsSinceLastMeaningfulProgression = secondsSinceProgression;
        possession.currentPhase = phase;
        possession.ballPosition = carrier.position;
        possession.ballProgression = progress;
        possession.recentProgression = progress - std::max(startProgress, lastMeaningfulProgress);
        possession.progressionAvailable = progressionLaneAvailable(
            carrier,
            teammates,
            opponents,
            shapeContext.attackingDirection,
            profile.forwardLaneMeters);
        possession.safeCirculationAvailable = safeCirculationAvailable(carrier, teammates, opponents);
        possession.finalThirdEntryAvailable = entryAvailable(
            carrier,
            teammates,
            opponents,
            shapeContext.attackingDirection,
            0.66);
        possession.boxEntryAvailable = entryAvailable(
            carrier,
            teammates,
            opponents,
            shapeContext.attackingDirection,
            profile.boxEntryProgress);
        possession.opponentBlockCompactness = blockCompactness;
        possession.localPressure = pressure;

        DefensiveContext defensive;
        defensive.defendingTeamId = shapeContext.teamId;
        defensive.opponentTeamId = state.possession.teamInPossession;
        defensive.secondsOutOfPossession = possession.secondsInPossession;
        defensive.opponentPossessionActionCount = state.possession.actionDepth;
        defensive.opponentPhase = phase;
        defensive.ballPosition = carrier.position;
        defensive.threatLevel = progress;
        defensive.localPressOpportunity = pressure;
        defensive.blockCompactness = blockCompactness;
        defensive.lineIntegrity = std::clamp(100.0 - blockCompactness * 0.35, 0.0, 100.0);
        defensive.pressTriggerActive =
            (secondsSinceProgression >= profile.staleProgressionSeconds
                && possession.safeCirculationAvailable)
            || (progress >= 0.66 && pressure >= 14.0)
            || state.possession.actionDepth >= static_cast<int>(profile.pressureBlockActionCount);
        defensive.counterPressAvailable = state.possession.isTransition;
        defensive.dangerInAssignedZone = progress >= 0.66;

        return PlayerDecisionContext{
            carrier.playerId,
            carrier.teamId,
            role,
            carrier.position,
            carrier.position,
            shapeContext.tacticalSetup,
            phase,
            possession,
            defensive,
            pressure
        };
    }

    DefensiveContext buildDefensiveContextForTeam(
        const MatchSimulationState& state,
        const TeamSimState& defendingTeam,
        const TeamSimState& opponentTeam,
        AttackingDirection defendingDirection) {
        DefensiveContext context;
        context.defendingTeamId = defendingTeam.teamId;
        context.opponentTeamId = opponentTeam.teamId;
        context.ballPosition = state.ball.position;
        context.opponentPossessionActionCount =
            state.possession.teamInPossession == opponentTeam.teamId
                ? state.possession.actionDepth
                : 0;
        context.secondsOutOfPossession =
            state.possession.teamInPossession == opponentTeam.teamId
                ? static_cast<double>(std::max(0, state.currentSecond - state.possession.possessionStartSecond))
                : 0.0;

        const AttackingDirection opponentDirection = attackingDirectionForTeam(opponentTeam);
        const double progress = ballProgression(state.ball.position, opponentDirection);
        const double lastProgress =
            ballProgression(state.possession.lastMeaningfulProgressionPoint, opponentDirection);
        const double secondsSinceProgression = static_cast<double>(
            std::max(0, state.currentSecond - state.possession.lastMeaningfulProgressionSecond));
        context.opponentPhase = decisionPhaseFor(
            state.ball.position,
            opponentDirection,
            state.possession.isTransition);
        context.threatLevel = progress * 100.0;
        context.localPressOpportunity = nearestOpponentPressure(state.ball.position, defendingTeam.players);
        context.blockCompactness = opponentBlockCompactness(state.ball.position, defendingTeam.players);
        context.lineIntegrity = std::clamp(100.0 - context.blockCompactness * 0.35, 0.0, 100.0);
        context.dangerInAssignedZone = progress >= 0.66;
        context.counterPressAvailable = state.possession.isTransition;

        const ContextProfile profile;
        const bool stalePossession =
            state.possession.teamInPossession == opponentTeam.teamId
            && context.opponentPossessionActionCount >= static_cast<int>(profile.pressureBlockActionCount)
            && secondsSinceProgression >= profile.staleProgressionSeconds
            && progress - lastProgress < 0.04;
        const bool dangerousPossession =
            state.possession.teamInPossession == opponentTeam.teamId
            && (progress >= 0.66 || context.localPressOpportunity >= 14.0);
        context.pressTriggerActive = stalePossession || dangerousPossession;
        (void)defendingDirection;
        return context;
    }

    AttackingDirection attackingDirectionForTeam(const TeamSimState& team) {
        return attackingDirectionFor(team.side == TeamSide::Home);
    }

    PitchPoint opponentGoalCenter(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    PitchPoint ownGoalCenter(const TeamSimState& team) {
        return team.side == TeamSide::Home
            ? PitchGeometry::homeGoalCenter()
            : PitchGeometry::awayGoalCenter();
    }

    double nearestOpponentPressure(
        PitchPoint position,
        const std::vector<PlayerSimState>& opponents) {
        double nearest = std::numeric_limits<double>::max();
        for (const PlayerSimState& opponent : opponents) {
            nearest = std::min(nearest, PitchGeometry::distance(position, opponent.position));
        }

        if (nearest <= 4.0) {
            return 35.0;
        }
        if (nearest <= 8.0) {
            return 22.0;
        }
        if (nearest <= 14.0) {
            return 10.0;
        }
        return opponents.empty() ? 0.0 : 3.0;
    }

    double ballProgression(PitchPoint point, AttackingDirection direction) {
        const double xProgress = direction == AttackingDirection::HomeToAway
            ? point.x / PitchGeometry::LengthMeters
            : (PitchGeometry::LengthMeters - point.x) / PitchGeometry::LengthMeters;
        return std::clamp(xProgress, 0.0, 1.0);
    }

    DecisionMatchPhase decisionPhaseFor(
        PitchPoint ballPosition,
        AttackingDirection direction,
        bool transition) {
        if (transition) {
            return DecisionMatchPhase::AttackingTransition;
        }

        const double progress = ballProgression(ballPosition, direction);
        if (progress >= 0.88) {
            return DecisionMatchPhase::ChanceCreation;
        }
        if (progress >= 0.78) {
            return DecisionMatchPhase::BoxEntry;
        }
        if (progress >= 0.66) {
            return DecisionMatchPhase::FinalThird;
        }
        if (progress >= 0.38) {
            return DecisionMatchPhase::MiddleThirdCirculation;
        }
        return DecisionMatchPhase::BuildUp;
    }

    double distancePointToSegment(PitchPoint point, PitchPoint start, PitchPoint end) {
        const double vx = end.x - start.x;
        const double vy = end.y - start.y;
        const double wx = point.x - start.x;
        const double wy = point.y - start.y;
        const double lengthSquared = vx * vx + vy * vy;
        if (lengthSquared <= 0.001) {
            return PitchGeometry::distance(point, start);
        }

        const double t = std::clamp(((wx * vx) + (wy * vy)) / lengthSquared, 0.0, 1.0);
        return PitchGeometry::distance(point, PitchPoint{ start.x + t * vx, start.y + t * vy });
    }

    double lanePressure(PitchPoint start, PitchPoint end, const std::vector<PlayerSimState>& opponents) {
        double pressure = 0.0;
        for (const PlayerSimState& opponent : opponents) {
            const double distance = distancePointToSegment(opponent.position, start, end);
            if (distance <= 3.0) {
                pressure += 24.0;
            } else if (distance <= 6.0) {
                pressure += 12.0;
            } else if (distance <= 9.0) {
                pressure += 5.0;
            }
        }
        return std::clamp(pressure, 0.0, 100.0);
    }

    double intentPressureContribution(PlayerIntentType intent) {
        switch (intent) {
        case PlayerIntentType::PressBallCarrier:
        case PlayerIntentType::AttemptTackle:
            return 18.0;
        case PlayerIntentType::ContainBallCarrier:
            return 13.0;
        case PlayerIntentType::BlockPassingLane:
        case PlayerIntentType::InterceptBallPath:
            return 15.0;
        case PlayerIntentType::MarkOpponent:
        case PlayerIntentType::CoverSpace:
            return 9.0;
        case PlayerIntentType::None:
        case PlayerIntentType::HoldAttackingShape:
        case PlayerIntentType::MoveToSupport:
        case PlayerIntentType::DropForPass:
        case PlayerIntentType::MakeRunBehind:
        case PlayerIntentType::AttackNearPost:
        case PlayerIntentType::AttackFarPost:
        case PlayerIntentType::AttackCutbackZone:
        case PlayerIntentType::ReceivePass:
        case PlayerIntentType::OccupyWidth:
        case PlayerIntentType::OccupyHalfSpace:
        case PlayerIntentType::HoldDefensiveShape:
        case PlayerIntentType::RecoverToGoal:
        case PlayerIntentType::ProtectGoalZone:
        case PlayerIntentType::RecoverLooseBall:
        case PlayerIntentType::ClearDanger:
            return 0.0;
        }
        return 0.0;
    }

    double contextualPassPressure(
        PitchPoint start,
        PitchPoint target,
        const std::vector<PlayerSimState>& defenders) {
        double pressure = 0.0;
        for (const PlayerSimState& defender : defenders) {
            const double contribution = intentPressureContribution(defender.currentIntent.type);
            if (contribution <= 0.0) {
                continue;
            }

            const double carrierDistance = PitchGeometry::distance(defender.position, start);
            const double targetDistance = PitchGeometry::distance(defender.position, target);
            const double laneDistance = distancePointToSegment(defender.position, start, target);
            if (carrierDistance <= 6.0) {
                pressure += contribution;
            } else if (targetDistance <= 5.5) {
                pressure += contribution * 0.85;
            } else if (laneDistance <= 5.0) {
                pressure += contribution * 0.75;
            }
        }
        return std::clamp(pressure, 0.0, 100.0);
    }

    double opponentBlockCompactness(
        PitchPoint ballPosition,
        const std::vector<PlayerSimState>& opponents) {
        const ContextProfile profile;
        if (opponents.empty()) {
            return 0.0;
        }

        int nearby = 0;
        double lateralSpread = 0.0;
        double averageY = 0.0;
        for (const PlayerSimState& opponent : opponents) {
            if (PitchGeometry::distance(ballPosition, opponent.position) <= profile.compactBlockRadius) {
                ++nearby;
                averageY += opponent.position.y;
            }
        }
        if (nearby == 0) {
            return 0.0;
        }

        averageY /= static_cast<double>(nearby);
        for (const PlayerSimState& opponent : opponents) {
            if (PitchGeometry::distance(ballPosition, opponent.position) <= profile.compactBlockRadius) {
                lateralSpread += std::abs(opponent.position.y - averageY);
            }
        }
        lateralSpread /= static_cast<double>(nearby);

        const double density = std::clamp(static_cast<double>(nearby) / 7.0, 0.0, 1.0);
        const double compactness = std::clamp(1.0 - (lateralSpread / 18.0), 0.0, 1.0);
        return std::clamp((density * 0.65 + compactness * 0.35) * 100.0, 0.0, 100.0);
    }

    bool safeCirculationAvailable(
        const PlayerSimState& carrier,
        const std::vector<PlayerSimState>& teammates,
        const std::vector<PlayerSimState>& opponents) {
        const ContextProfile profile;
        for (const PlayerSimState& teammate : teammates) {
            if (teammate.playerId == carrier.playerId) {
                continue;
            }
            if (PitchGeometry::distance(carrier.position, teammate.position) > profile.safePassDistance) {
                continue;
            }
            if (nearestOpponentPressure(teammate.position, opponents) <= profile.safeReceiverPressureLimit
                && lanePressure(carrier.position, teammate.position, opponents) <= 36.0) {
                return true;
            }
        }
        return false;
    }

    bool progressionLaneAvailable(
        const PlayerSimState& carrier,
        const std::vector<PlayerSimState>& teammates,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction,
        double requiredForwardMeters) {
        for (const PlayerSimState& teammate : teammates) {
            if (teammate.playerId == carrier.playerId) {
                continue;
            }
            const double forward =
                (teammate.position.x - carrier.position.x)
                * (direction == AttackingDirection::HomeToAway ? 1.0 : -1.0);
            if (forward < requiredForwardMeters) {
                continue;
            }
            if (lanePressure(carrier.position, teammate.position, opponents) <= 44.0) {
                return true;
            }
        }

        const PitchPoint carryProbe = PitchGeometry::clampToPitch(PitchPoint{
            carrier.position.x + (direction == AttackingDirection::HomeToAway ? requiredForwardMeters : -requiredForwardMeters),
            carrier.position.y
        });
        return nearestOpponentPressure(carryProbe, opponents) <= 14.0;
    }

    bool entryAvailable(
        const PlayerSimState& carrier,
        const std::vector<PlayerSimState>& teammates,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction,
        double entryProgress) {
        for (const PlayerSimState& teammate : teammates) {
            if (teammate.playerId == carrier.playerId) {
                continue;
            }
            if (ballProgression(teammate.position, direction) >= entryProgress
                && lanePressure(carrier.position, teammate.position, opponents) <= 48.0) {
                return true;
            }
        }
        return false;
    }

    void updateMeaningfulProgression(MatchSimulationState& state) {
        const TeamSimState* team = findTeamState(state, state.possession.teamInPossession);
        if (team == nullptr) {
            return;
        }

        const ContextProfile profile;
        const AttackingDirection direction = attackingDirectionForTeam(*team);
        const double currentProgress = ballProgression(state.ball.position, direction);
        const double lastProgress =
            ballProgression(state.possession.lastMeaningfulProgressionPoint, direction);
        if ((currentProgress - lastProgress) * PitchGeometry::LengthMeters >= profile.meaningfulProgressMeters) {
            state.possession.lastMeaningfulProgressionPoint = state.ball.position;
            state.possession.lastMeaningfulProgressionSecond = state.currentSecond;
        }
    }

    bool isInsideGoalMouthY(double y) {
        const double halfGoalWidth = PitchGeometry::GoalWidthMeters / 2.0;
        const double centerY = PitchGeometry::WidthMeters / 2.0;
        return y >= centerY - halfGoalWidth && y <= centerY + halfGoalWidth;
    }

    bool shotCrossesGoalMouth(
        const BallTrajectory& trajectory,
        AttackingDirection attackingDirection) {
        if (!isInsideGoalMouthY(trajectory.actualTarget.y)) {
            return false;
        }

        if (attackingDirection == AttackingDirection::HomeToAway) {
            return trajectory.actualTarget.x >= PitchGeometry::LengthMeters - 1.0;
        }

        return trajectory.actualTarget.x <= 1.0;
    }

    double goalkeeperStrengthFor(const MatchEngineInput& input, const PlayerSimState* goalkeeper) {
        if (goalkeeper == nullptr) {
            return 45.0;
        }
        const MatchPlayerSnapshot* snapshot = findSnapshotForPlayer(input, goalkeeper->playerId);
        if (snapshot == nullptr) {
            return static_cast<double>(goalkeeper->baseOverall);
        }

        return clampDouble(
            snapshot->attributes.goalkeeper.shotStopping * 0.36
                + snapshot->attributes.goalkeeper.oneOnOnes * 0.22
                + snapshot->attributes.goalkeeper.handling * 0.18
                + snapshot->attributes.goalkeeper.aerialAbility * 0.10
                + snapshot->attributes.mental.concentration * 0.08
                + snapshot->baseOverall * 0.06,
            25.0,
            95.0);
    }

    PlayerAttributes attributesForPlayer(
        const MatchEngineInput& input,
        PlayerId playerId) {
        const MatchPlayerSnapshot* snapshot = findSnapshotForPlayer(input, playerId);
        return snapshot != nullptr ? snapshot->attributes : PlayerAttributes{};
    }

    std::vector<PitchPoint> defenderPositionsFor(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam) {
        std::vector<PitchPoint> positions;
        positions.reserve(defendingTeam.players.size());
        for (const PlayerSimState& defender : defendingTeam.players) {
            if (isAssignedGoalkeeper(input, defendingTeam.teamId, defender.playerId)) {
                continue;
            }
            positions.push_back(defender.position);
        }
        return positions;
    }

    std::vector<ShotBlocker> shotBlockersFor(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam) {
        std::vector<ShotBlocker> blockers;
        blockers.reserve(defendingTeam.players.size());
        for (const PlayerSimState& defender : defendingTeam.players) {
            if (isAssignedGoalkeeper(input, defendingTeam.teamId, defender.playerId)) {
                continue;
            }
            blockers.push_back(ShotBlocker{
                defender.playerId,
                defender.teamId,
                defender.position,
                attributesForPlayer(input, defender.playerId),
                defender.baseOverall
            });
        }
        return blockers;
    }

    double goalkeeperHandlingFor(
        const MatchEngineInput& input,
        const PlayerSimState* goalkeeper) {
        if (goalkeeper == nullptr) {
            return 45.0;
        }
        const MatchPlayerSnapshot* snapshot = findSnapshotForPlayer(input, goalkeeper->playerId);
        return snapshot != nullptr
            ? clampedAttribute(snapshot->attributes.goalkeeper.handling)
            : clampedAttribute(goalkeeper->baseOverall);
    }

    bool isHighBallTrajectory(const BallTrajectory& trajectory) {
        return trajectory.type == BallTrajectoryType::HighCross
            || trajectory.type == BallTrajectoryType::Clearance
            || trajectory.flightProfile == BallFlightProfile::High
            || trajectory.flightProfile == BallFlightProfile::Lofted;
    }

    bool canReachBallHeight(
        const MatchEngineInput& input,
        const PlayerSimState& player,
        const BallTrajectory& trajectory,
        double second) {
        const MatchPlayerSnapshot* snapshot = findSnapshotForPlayer(input, player.playerId);
        const PlayerAttributes attributes =
            snapshot != nullptr ? snapshot->attributes : PlayerAttributes{};
        const bool goalkeeper = isAssignedGoalkeeper(input, player.teamId, player.playerId);
        const double heightMeters = ballHeightAtSecond(trajectory, second);

        if (heightMeters <= effectiveGroundInterventionReachMeters(attributes)) {
            return true;
        }

        return heightMeters <= effectiveAerialReachMeters(attributes, goalkeeper);
    }

    std::optional<InterceptionCandidate> bestReachableInterceptionCandidate(
        const MatchSimulationState& state,
        const MatchEngineInput& input,
        const BallTrajectory& trajectory,
        const InterceptionResolverResult& interception,
        double minimumQualityScore) {
        std::optional<InterceptionCandidate> best;

        for (const InterceptionCandidate& candidate : interception.candidates) {
            if (candidate.qualityScore < minimumQualityScore) {
                continue;
            }

            const PlayerSimState* defender = findPlayerState(state, candidate.playerId);
            if (defender == nullptr
                || !canReachBallHeight(input, *defender, trajectory, candidate.ballArrivalSecond)) {
                continue;
            }

            if (!best || candidate.qualityScore > best->qualityScore) {
                best = candidate;
            }
        }

        return best;
    }

    const PlayerSimState* findGoalkeeperOrNearestOwnGoal(
        const MatchEngineInput& input,
        const TeamSimState& team) {
        const MatchTeamSnapshot* snapshot = snapshotForTeam(input, team.teamId);
        if (snapshot != nullptr) {
            for (const TeamSheetSlotAssignment& assignment : snapshot->teamSheet.startingAssignments) {
                if (assignment.slotRole == FormationSlotRole::Goalkeeper) {
                    if (const PlayerSimState* goalkeeper = findPlayerInTeam(team, assignment.playerId)) {
                        return goalkeeper;
                    }
                }
            }
        }

        const PlayerSimState* nearest = nullptr;
        double nearestDistance = std::numeric_limits<double>::max();
        const PitchPoint goalCenter = ownGoalCenter(team);
        for (const PlayerSimState& player : team.players) {
            const double distance = PitchGeometry::distance(player.position, goalCenter);
            if (nearest == nullptr || distance < nearestDistance) {
                nearest = &player;
                nearestDistance = distance;
            }
        }

        return nearest;
    }

    void resetForLocalKickoff(
        MatchSimulationState& state,
        const MatchEngineInput& input,
        TeamId concedingTeamId) {
        TeamSimState* concedingTeam = findTeamState(state, concedingTeamId);
        const MatchTeamSnapshot* concedingSnapshot = snapshotForTeam(input, concedingTeamId);
        if (concedingTeam == nullptr || concedingSnapshot == nullptr) {
            setLooseBall(state, pitchCenter());
            state.possession.isTransition = false;
            return;
        }

        const PlayerId kickoffPlayerId = chooseKickoffCarrier(*concedingSnapshot, *concedingTeam);
        setControlledBy(state, kickoffPlayerId, concedingTeamId, pitchCenter());
        state.possession.phase = PossessionPhase::BuildUp;
        state.possession.isTransition = false;
    }

    int eventMinuteForSecond(int second) {
        return std::max(1, (second / 60) + 1);
    }

    void appendOfficialGoalEvent(
        MatchEngineResult& result,
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        PlayerId assistPlayerId) {
        result.events.push_back(MatchEventRecord{
            eventMinuteForSecond(state.currentSecond),
            MatchEventKind::Goal,
            pending.sourceTeamId,
            pending.sourcePlayerId,
            assistPlayerId
        });
    }

    PlayerId assistForGoal(
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        const AssistTracker& assistTracker) {
        if (assistTracker.teamId != pending.sourceTeamId
            || assistTracker.passerPlayerId == 0
            || assistTracker.passerPlayerId == pending.sourcePlayerId
            || state.currentSecond - assistTracker.second > 20) {
            return 0;
        }
        return assistTracker.passerPlayerId;
    }

    void rememberCompletedPass(
        AssistTracker& assistTracker,
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        PlayerId receiverPlayerId) {
        if (!isPassLike(pending.actionType)
            || pending.sourceTeamId == 0
            || pending.sourcePlayerId == 0
            || receiverPlayerId == 0) {
            return;
        }

        assistTracker.passerPlayerId = pending.sourcePlayerId;
        assistTracker.receiverPlayerId = receiverPlayerId;
        assistTracker.teamId = pending.sourceTeamId;
        assistTracker.second = state.currentSecond;
    }

    double trajectoryElapsedSeconds(const BallTrajectory& trajectory) {
        return clampElapsedSeconds(trajectory.arrivalSecond - trajectory.startSecond);
    }

    double remainingTrajectorySeconds(const MatchSimulationState& state, const BallTrajectory& trajectory) {
        return clampElapsedSeconds(trajectory.arrivalSecond - static_cast<double>(state.currentSecond));
    }

    double movementDurationSeconds(
        PitchPoint start,
        PitchPoint target,
        double effectivePace,
        double minimum,
        double maximum) {
        const double speed = std::max(1.0, effectivePace);
        return clampElapsedSeconds(PitchGeometry::distance(start, target) / speed, minimum, maximum);
    }

    double receptionControlRange(
        BallCarrierActionType passType,
        double executionQuality,
        const MatchPlayerSnapshot* receiver,
        double pressure) {
        double minimum = 3.0;
        double maximum = 5.0;
        switch (passType) {
        case BallCarrierActionType::BackPass:
        case BallCarrierActionType::ShortPass:
            minimum = 3.5;
            maximum = 5.5;
            break;
        case BallCarrierActionType::ThroughBall:
            minimum = 2.5;
            maximum = 4.0;
            break;
        case BallCarrierActionType::SwitchPlay:
            minimum = 3.0;
            maximum = 5.0;
            break;
        case BallCarrierActionType::LowCross:
        case BallCarrierActionType::Cutback:
        case BallCarrierActionType::HighCross:
            minimum = 1.8;
            maximum = 3.2;
            break;
        case BallCarrierActionType::Carry:
        case BallCarrierActionType::Dribble:
        case BallCarrierActionType::CutInside:
        case BallCarrierActionType::Shoot:
        case BallCarrierActionType::Hold:
        case BallCarrierActionType::Clear:
            break;
        }

        const PlayerAttributes attributes =
            receiver != nullptr ? receiver->attributes : PlayerAttributes{};
        const double receptionSkill = attributeAverage({
            clampedAttribute(attributes.technical.firstTouch),
            clampedAttribute(attributes.technical.technique),
            clampedAttribute(attributes.mental.composure)
        });
        const double qualityAdjustment = (executionQuality - 60.0) / 30.0;
        const double skillAdjustment = (receptionSkill - 60.0) / 28.0;
        const double pressureAdjustment = -(clampDouble(pressure, 0.0, 100.0) / 45.0);
        return clampDouble(
            ((minimum + maximum) * 0.5) + qualityAdjustment + skillAdjustment + pressureAdjustment,
            minimum,
            maximum);
    }

    double interceptionQualityThresholdFor(BallCarrierActionType actionType) {
        switch (actionType) {
        case BallCarrierActionType::BackPass:
            return 26.0;
        case BallCarrierActionType::ShortPass:
            return 22.0;
        case BallCarrierActionType::SwitchPlay:
            return 18.0;
        case BallCarrierActionType::ThroughBall:
            return 14.0;
        case BallCarrierActionType::LowCross:
        case BallCarrierActionType::Cutback:
            return 12.0;
        case BallCarrierActionType::HighCross:
            return 11.0;
        case BallCarrierActionType::Shoot:
            return 8.0;
        case BallCarrierActionType::Carry:
        case BallCarrierActionType::Dribble:
        case BallCarrierActionType::CutInside:
        case BallCarrierActionType::Hold:
        case BallCarrierActionType::Clear:
            return 16.0;
        }

        return 16.0;
    }

    double interceptionReactionWindowFor(BallCarrierActionType actionType) {
        switch (actionType) {
        case BallCarrierActionType::BackPass:
            return 0.10;
        case BallCarrierActionType::ShortPass:
            return 0.12;
        case BallCarrierActionType::SwitchPlay:
            return 0.22;
        case BallCarrierActionType::ThroughBall:
            return 0.35;
        case BallCarrierActionType::LowCross:
        case BallCarrierActionType::Cutback:
        case BallCarrierActionType::HighCross:
        case BallCarrierActionType::Shoot:
            return 0.35;
        case BallCarrierActionType::Carry:
        case BallCarrierActionType::Dribble:
        case BallCarrierActionType::CutInside:
        case BallCarrierActionType::Hold:
        case BallCarrierActionType::Clear:
            return 0.25;
        }

        return 0.25;
    }

    void moveReceiverTowardPass(
        MatchSimulationState& state,
        const MatchEngineInput& input,
        const PendingBallAction& pending,
        const BallTrajectory& trajectory) {
        if (!isPassLike(pending.actionType) || pending.targetPlayerId == 0) {
            return;
        }

        PlayerSimState* receiver = findPlayerState(state, pending.targetPlayerId);
        if (receiver == nullptr || receiver->teamId != pending.sourceTeamId) {
            return;
        }

        const MatchPlayerSnapshot* receiverSnapshot =
            findSnapshotForPlayer(input, receiver->playerId);
        const PlayerAttributes attributes =
            receiverSnapshot != nullptr ? receiverSnapshot->attributes : PlayerAttributes{};
        const double receptionSkill = attributeAverage({
            clampedAttribute(attributes.technical.firstTouch),
            clampedAttribute(attributes.mental.offTheBall),
            clampedAttribute(attributes.mental.decisions)
        });
        const double elapsedSeconds =
            std::max(0.25, trajectory.arrivalSecond - static_cast<double>(state.currentSecond));
        const double maxDistance =
            receiver->effectivePace * elapsedSeconds * (1.05 + ((receptionSkill - 60.0) / 220.0));
        const double distance = PitchGeometry::distance(receiver->position, trajectory.actualTarget);
        if (distance <= 0.001) {
            return;
        }

        const double ratio = std::min(1.0, maxDistance / distance);
        receiver->position = PitchGeometry::clampToPitch(PitchPoint{
            receiver->position.x + ((trajectory.actualTarget.x - receiver->position.x) * ratio),
            receiver->position.y + ((trajectory.actualTarget.y - receiver->position.y) * ratio)
        });
        receiver->targetPosition = trajectory.actualTarget;
        receiver->currentIntent = PlayerIntent{
            PlayerIntentType::ReceivePass,
            trajectory.actualTarget,
            pending.sourcePlayerId,
            1.0
        };
    }

    SimulationStepResult applyLocalGoal(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const PendingBallAction& pending,
        const BallTrajectory& trajectory,
        TeamId concedingTeamId,
        AssistTracker& assistTracker) {
        const PlayerId assistPlayerId = assistForGoal(state, pending, assistTracker);
        ++teamStatsFor(result, pending.sourceTeamId).goals;
        ++playerStatsFor(result, pending.sourcePlayerId, pending.sourceTeamId).goals;
        if (assistPlayerId != 0) {
            ++playerStatsFor(result, assistPlayerId, pending.sourceTeamId).assists;
        }
        appendOfficialGoalEvent(result, state, pending, assistPlayerId);
        clearAssist(assistTracker);

        if (TeamSimState* scoringTeam = findTeamState(state, pending.sourceTeamId)) {
            ++scoringTeam->goals;
        }

        const AttackingDirection direction =
            state.homeTeam.teamId == pending.sourceTeamId
                ? AttackingDirection::HomeToAway
                : AttackingDirection::AwayToHome;
        appendTrace(
            result,
            input.options.detail,
            state,
            MatchTraceKind::Goal,
            pending.sourceTeamId,
            pending.sourcePlayerId,
            0,
            trajectory.actualTarget,
            opponentGoalCenter(direction));

        resetForLocalKickoff(state, input, concedingTeamId);
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
        return SimulationStepResult{ RestartAfterGoalSeconds };
    }

    SimulationStepResult executeControlledAction(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const TeamShapeContext& carrierShapeContext,
        const BallTrajectoryBuilder& trajectoryBuilder,
        const BallCarrierDecisionModel& decisionModel,
        const ActionSelector& selector,
        std::optional<PendingBallAction>& pending,
        std::uint64_t baseSeed,
        double deltaSeconds) {
        PlayerSimState* carrier = findPlayerState(state, state.ball.carrierPlayerId);
        if (carrier == nullptr) {
            setLooseBall(state, state.ball.position);
            return SimulationStepResult{ 1.0 };
        }

        state.ball.position = carrier->position;
        const MatchTeamSnapshot* carrierSnapshot = snapshotForTeam(input, carrier->teamId);
        const MatchPlayerSnapshot* playerSnapshot =
            findSnapshotForPlayer(input, carrier->playerId);
        const TeamSimState* carrierTeamState = findTeamState(state, carrier->teamId);
        const TeamSimState* opponentState = opponentTeam(state, carrier->teamId);
        if (carrierSnapshot == nullptr || carrierTeamState == nullptr || opponentState == nullptr) {
            return SimulationStepResult{ 1.0 };
        }

        const FormationSlotRole carrierRole = assignedRoleFor(
            carrierSnapshot->teamSheet,
            carrier->playerId);
        const PlayerDecisionContext playerDecisionContext = buildPlayerDecisionContext(
            state,
            carrierShapeContext,
            *carrier,
            carrierRole,
            carrierTeamState->players,
            opponentState->players);
        const std::vector<ActionCandidate> candidates = decisionModel.evaluate(
            BallCarrierDecisionModelContext{
                carrierSnapshot,
                snapshotForTeam(input, opponentState->teamId),
                carrierTeamState,
                opponentState,
                carrier,
                carrierShapeContext.attackingDirection,
                playerDecisionContext
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
            return SimulationStepResult{ 1.0 };
        }

        const ActionCandidate action = *selection.selected;
        const BallCarrierActionType actionType = action.type;
        ++state.possession.actionDepth;
        const double executionPressure = isPassLike(actionType)
            ? std::max(
                action.pressurePenalty,
                contextualPassPressure(
                    state.ball.position,
                    action.intendedTarget,
                    opponentState->players))
            : action.pressurePenalty;
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
            const double holdSeconds = 1.0
                + matchEngineDeterministicUnitInterval(
                    stepSeed(baseSeed, state, carrier->playerId ^ 0x681dULL)) * 2.0;
            return SimulationStepResult{ clampElapsedSeconds(holdSeconds, 1.0, 3.0), true };
        }

        if (actionType == BallCarrierActionType::Carry
            || actionType == BallCarrierActionType::Dribble
            || actionType == BallCarrierActionType::CutInside) {
            const double elapsedSeconds = movementDurationSeconds(
                carrier->position,
                action.intendedTarget,
                carrier->effectivePace,
                2.0,
                actionType == BallCarrierActionType::Carry ? 6.0 : 5.0);
            moveControlledBall(
                state,
                *carrier,
                action.intendedTarget,
                (actionType == BallCarrierActionType::Carry ? 5.5 : 4.0) * elapsedSeconds);
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
            return SimulationStepResult{ elapsedSeconds, true };
        }

        const BallTrajectoryType trajectoryType = trajectoryTypeFor(actionType);
        double executionQuality = executionQualityFor(playerSnapshot, actionType);
        double shotXG = 0.0;
        ShotType shotType = ShotType::ControlledFinish;
        ShotContext shotContext;
        ShotTargetSelectionResult shotTarget;
        ShotExecutionResult shotExecution;
        ShotQualityResult shotQuality;
        BallTrajectory trajectory;

        if (isPassLike(actionType)) {
            const BallTrajectoryBuildResult trajectoryResult = trajectoryBuilder.build(BallTrajectoryBuildRequest{
                state.ball.position,
                action.intendedTarget,
                trajectoryType,
                static_cast<double>(state.currentSecond),
                executionQuality,
                executionPressure,
                stepSeed(
                    baseSeed,
                    state,
                    static_cast<std::uint64_t>(carrier->playerId)
                        ^ (static_cast<std::uint64_t>(actionType) << 32))
            });
            trajectory = trajectoryResult.trajectory;
            ++teamStatsFor(result, carrier->teamId).passesAttempted;
            ++playerStatsFor(result, carrier->playerId, carrier->teamId).passesAttempted;
        } else if (actionType == BallCarrierActionType::Shoot) {
            const PlayerSimState* goalkeeper = findGoalkeeperOrNearestOwnGoal(input, *opponentState);
            const std::uint64_t shotSeed = stepSeed(
                baseSeed,
                state,
                static_cast<std::uint64_t>(carrier->playerId)
                    ^ (static_cast<std::uint64_t>(actionType) << 32)
                    ^ 0x51f00dULL);
            shotContext = ShotContextBuilder{}.build(ShotContextBuildRequest{
                state.ball.position,
                carrierShapeContext.attackingDirection,
                executionPressure,
                playerSnapshot != nullptr ? playerSnapshot->attributes : PlayerAttributes{},
                goalkeeper != nullptr ? attributesForPlayer(input, goalkeeper->playerId) : PlayerAttributes{},
                goalkeeperStrengthFor(input, goalkeeper),
                defenderPositionsFor(input, *opponentState),
                shotSeed
            });
            const ShotTypeSelectionResult selectedShotType = ShotTypeSelector{}.select(shotContext);
            shotType = selectedShotType.type;
            shotTarget = ShotTargetSelector{}.select(shotContext, shotType);
            shotExecution = ShotExecutionModel{}.execute(ShotExecutionRequest{
                shotContext,
                shotType,
                shotTarget
            });
            shotQuality = ShotQualityModel{}.evaluate(shotContext, shotType, shotExecution);
            trajectory = ShotTrajectoryBuilder{}.build(ShotTrajectoryBuildRequest{
                shotContext,
                shotTarget,
                shotExecution,
                static_cast<double>(state.currentSecond)
            });
            executionQuality = shotExecution.executionQuality;
            shotXG = shotQuality.adjustedXG;
            ++teamStatsFor(result, carrier->teamId).shots;
            teamStatsFor(result, carrier->teamId).expectedGoals += shotXG;
            ++playerStatsFor(result, carrier->playerId, carrier->teamId).shots;
        } else {
            const BallTrajectoryBuildResult trajectoryResult = trajectoryBuilder.build(BallTrajectoryBuildRequest{
                state.ball.position,
                action.intendedTarget,
                trajectoryType,
                static_cast<double>(state.currentSecond),
                executionQuality,
                executionPressure,
                stepSeed(
                    baseSeed,
                    state,
                    static_cast<std::uint64_t>(carrier->playerId)
                        ^ (static_cast<std::uint64_t>(actionType) << 32))
            });
            trajectory = trajectoryResult.trajectory;
        }

        clearBallFlags(state);
        state.ball.controlState = BallControlState::InFlight;
        state.ball.carrierPlayerId = 0;
        state.ball.carrierTeamId = 0;
        state.ball.position = trajectory.start;
        state.ball.trajectory = trajectory;
        state.possession.teamInPossession = 0;
        state.possession.lastPossessionTeamId = carrier->teamId;
        state.possession.ballCarrierId = 0;

        pending = pendingPlayerAction(
            carrier->teamId,
            carrier->playerId,
            action.targetPlayerId,
            actionType,
            trajectoryType,
            executionQuality,
            executionPressure,
            actionType == BallCarrierActionType::Shoot,
            shotXG,
            shotType,
            shotContext,
            shotTarget,
            shotExecution,
            shotQuality);

        appendTrace(
            result,
            input.options.detail,
            state,
            traceKindFor(actionType),
            carrier->teamId,
            carrier->playerId,
            action.targetPlayerId,
            trajectory.start,
            trajectory.actualTarget);
        return SimulationStepResult{ trajectoryElapsedSeconds(trajectory), true };
    }

    ContestType contestTypeFor(
        const PendingBallAction& pending,
        const BallTrajectory& trajectory) {
        if (pending.isShot) {
            return ContestType::ShotBlock;
        }
        if (isHighBallTrajectory(trajectory)) {
            return ContestType::AerialDuel;
        }
        if (pending.trajectoryType == BallTrajectoryType::LowCross
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

    PlayerSimState* bestArrivalContestDefender(
        TeamSimState& defendingTeam,
        const PendingBallAction& pending,
        const BallTrajectory& trajectory,
        const PlayerSimState& receiver) {
        PlayerSimState* best = nullptr;
        double bestScore = 0.0;
        for (PlayerSimState& defender : defendingTeam.players) {
            const double receiverDistance = PitchGeometry::distance(defender.position, receiver.position);
            const double arrivalDistance = PitchGeometry::distance(defender.position, trajectory.actualTarget);
            const double laneDistance =
                distancePointToSegment(defender.position, trajectory.start, trajectory.actualTarget);
            if (receiverDistance > 9.0 && arrivalDistance > 9.0 && laneDistance > 7.0) {
                continue;
            }

            const double intent = intentPressureContribution(defender.currentIntent.type);
            const double score =
                intent
                + std::max(0.0, 18.0 - receiverDistance * 2.0)
                + std::max(0.0, 18.0 - arrivalDistance * 2.0)
                + std::max(0.0, 12.0 - laneDistance * 1.8)
                + pending.pressure * 0.12;
            if (score > bestScore) {
                best = &defender;
                bestScore = score;
            }
        }
        return best;
    }

    void recordDefenderPassWin(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const PendingBallAction& pending,
        PlayerSimState& defender,
        PitchPoint winPoint,
        AssistTracker& assistTracker) {
        setControlledBy(state, defender.playerId, defender.teamId, winPoint);
        ++teamStatsFor(result, defender.teamId).interceptions;
        ++playerStatsFor(result, defender.playerId, defender.teamId).interceptions;
        if (isPassLike(pending.actionType)) {
            ++teamStatsFor(result, pending.sourceTeamId).passesIntercepted;
        }
        clearAssist(assistTracker);
        appendTrace(
            result,
            input.options.detail,
            state,
            MatchTraceKind::Turnover,
            defender.teamId,
            defender.playerId,
            pending.sourcePlayerId,
            winPoint,
            winPoint);
        appendTrace(
            result,
            input.options.detail,
            state,
            MatchTraceKind::Interception,
            defender.teamId,
            defender.playerId,
            pending.sourcePlayerId,
            winPoint,
            winPoint);
    }

    void recordLoosePass(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const PendingBallAction& pending,
        PitchPoint loosePoint,
        AssistTracker& assistTracker) {
        if (isPassLike(pending.actionType)) {
            ++teamStatsFor(result, pending.sourceTeamId).passesLoose;
        }
        setLooseBall(state, loosePoint);
        clearAssist(assistTracker);
        appendTrace(
            result,
            input.options.detail,
            state,
            MatchTraceKind::LooseBall,
            pending.sourceTeamId,
            pending.sourcePlayerId,
            0,
            loosePoint,
            loosePoint);
    }

    SimulationStepResult processShotAtGoal(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        std::optional<PendingBallAction>& pending,
        const BallTrajectory& trajectory,
        const TeamSimState& defendingTeam,
        std::uint64_t baseSeed,
        AssistTracker& assistTracker) {
        const double elapsedToShot = remainingTrajectorySeconds(state, trajectory);
        if (!pending || !pending->isShot) {
            return SimulationStepResult{ elapsedToShot };
        }

        const TeamSimState* attackingTeam = findTeamState(state, pending->sourceTeamId);
        if (attackingTeam == nullptr) {
            setLooseBall(state, trajectory.actualTarget);
            clearAssist(assistTracker);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToShot };
        }

        const AttackingDirection direction = attackingDirectionForTeam(*attackingTeam);
        const bool crossesGoalMouth = shotCrossesGoalMouth(trajectory, direction);
        const PlayerSimState* goalkeeper = findGoalkeeperOrNearestOwnGoal(input, defendingTeam);
        const ShootingModelTuning shootingTuning;
        ShotOutcomeResult outcome = crossesGoalMouth
            ? ShotOutcomeResolver{}.resolve(ShotOutcomeContext{
                pending->shotContext,
                pending->shotType,
                pending->shotExecution,
                pending->shotQuality,
                stepSeed(baseSeed, state, pending->sourcePlayerId ^ 0x9010ULL)
            })
            : ShotOutcomeResult{ ShotOutcomeKind::OffTarget, false, false, false };

        if (!outcome.onTarget) {
            clearAssist(assistTracker);
            if (goalkeeper != nullptr) {
                setControlledBy(state, goalkeeper->playerId, goalkeeper->teamId, goalkeeper->position);
                appendTrace(
                    result,
                    input.options.detail,
                    state,
                    MatchTraceKind::PossessionStart,
                    goalkeeper->teamId,
                    goalkeeper->playerId,
                    pending->sourcePlayerId,
                    goalkeeper->position,
                    goalkeeper->position);
                pending = std::nullopt;
                return SimulationStepResult{
                    elapsedToShot + shootingTuning.flow.offTargetRestartSeconds
                };
            }

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
            return SimulationStepResult{
                elapsedToShot + shootingTuning.flow.offTargetRestartSeconds
            };
        }

        ++teamStatsFor(result, pending->sourceTeamId).shotsOnTarget;

        const PitchPoint savePoint = trajectory.actualTarget;
        if (outcome.goal) {
            const SimulationStepResult goalStep = applyLocalGoal(
                state,
                result,
                input,
                *pending,
                trajectory,
                defendingTeam.teamId,
                assistTracker);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToShot + goalStep.elapsedSeconds };
        }

        if (outcome.kind == ShotOutcomeKind::SavedHeld && goalkeeper != nullptr) {
            setControlledBy(state, goalkeeper->playerId, goalkeeper->teamId, savePoint);
            clearAssist(assistTracker);
            appendTrace(
                result,
                input.options.detail,
                state,
                MatchTraceKind::Save,
                goalkeeper->teamId,
                goalkeeper->playerId,
                pending->sourcePlayerId,
                savePoint,
                savePoint);
            pending = std::nullopt;
            return SimulationStepResult{
                elapsedToShot + shootingTuning.flow.savedHeldRestartSeconds
            };
        }

        if (outcome.kind == ShotOutcomeKind::SavedRebound && goalkeeper != nullptr) {
            state.ball.controlState = BallControlState::InFlight;
            state.ball.carrierPlayerId = 0;
            state.ball.carrierTeamId = 0;
            state.ball.position = savePoint;
            state.possession.teamInPossession = 0;
            state.possession.lastPossessionTeamId = goalkeeper->teamId;
            state.possession.ballCarrierId = 0;
            state.ball.trajectory = ReboundTrajectoryBuilder{}.build(
                ReboundTrajectoryRequest{
                    savePoint,
                    trajectory,
                    pending->shotExecution,
                    ShotOutcomeKind::SavedRebound,
                    goalkeeperHandlingFor(input, goalkeeper),
                    0.0,
                    trajectory.arrivalSecond,
                    stepSeed(baseSeed, state, goalkeeper->playerId ^ 0x5afeULL)
                });
            appendTrace(
                result,
                input.options.detail,
                state,
                MatchTraceKind::Save,
                goalkeeper->teamId,
                goalkeeper->playerId,
                pending->sourcePlayerId,
                savePoint,
                state.ball.trajectory->actualTarget);
            pending = pendingUncontrolledBall(
                PendingBallKind::SavedRebound,
                goalkeeper->teamId,
                goalkeeper->playerId,
                BallTrajectoryType::Rebound);
            clearAssist(assistTracker);
            return SimulationStepResult{ elapsedToShot };
        }

        clearAssist(assistTracker);
        setLooseBall(state, savePoint);
        appendTrace(
            result,
            input.options.detail,
            state,
            MatchTraceKind::Save,
            goalkeeper != nullptr ? goalkeeper->teamId : defendingTeam.teamId,
            goalkeeper != nullptr ? goalkeeper->playerId : 0,
            pending->sourcePlayerId,
            savePoint,
            savePoint);
        pending = std::nullopt;
        return SimulationStepResult{ elapsedToShot };
    }

    SimulationStepResult processInFlightBall(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const BallTrajectoryBuilder& trajectoryBuilder,
        const InterceptionResolver& interceptionResolver,
        const ContestResolver& contestResolver,
        std::optional<ContestResolverResult>& lastContestResult,
        std::optional<PendingBallAction>& pending,
        std::uint64_t baseSeed,
        AssistTracker& assistTracker) {
        if (!state.ball.trajectory) {
            setLooseBall(state, state.ball.position);
            pending = std::nullopt;
            return SimulationStepResult{ 1.0 };
        }

        const BallTrajectory trajectory = *state.ball.trajectory;
        const double elapsedToArrival = remainingTrajectorySeconds(state, trajectory);
        if (pending && pending->kind != PendingBallKind::PlayerAction) {
            state.ball.position = PitchGeometry::clampToPitch(trajectory.actualTarget);
            setLooseBall(state, state.ball.position);
            state.possession.lastPossessionTeamId = pending->sourceTeamId;
            clearAssist(assistTracker);
            appendTrace(
                result,
                input.options.detail,
                state,
                MatchTraceKind::LooseBall,
                pending->sourceTeamId,
                pending->sourcePlayerId,
                0,
                state.ball.position,
                state.ball.position);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToArrival };
        }

        const TeamId attackingTeamId = pending
            ? pending->sourceTeamId
            : state.possession.lastPossessionTeamId;
        TeamSimState* defendingTeam = opponentTeam(state, attackingTeamId);
        if (defendingTeam == nullptr) {
            setLooseBall(state, trajectory.actualTarget);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToArrival };
        }

        if (pending) {
            moveReceiverTowardPass(state, input, *pending, trajectory);
        }

        if (pending && isPassLike(pending->actionType)) {
            state.ball.position = PitchGeometry::clampToPitch(trajectory.actualTarget);
            PlayerSimState* target = findPlayerState(state, pending->targetPlayerId);
            const MatchPlayerSnapshot* targetSnapshot =
                target != nullptr ? findSnapshotForPlayer(input, target->playerId) : nullptr;
            const double controlRange = receptionControlRange(
                pending->actionType,
                pending->executionQuality,
                targetSnapshot,
                pending->pressure);

            if (target == nullptr) {
                recordLoosePass(
                    state,
                    result,
                    input,
                    *pending,
                    state.ball.position,
                    assistTracker);
                pending = std::nullopt;
                return SimulationStepResult{ elapsedToArrival };
            }

            PlayerSimState* arrivalDefender =
                bestArrivalContestDefender(*defendingTeam, *pending, trajectory, *target);
            const double receiverDistanceToArrival =
                PitchGeometry::distance(target->position, state.ball.position);
            if (receiverDistanceToArrival > controlRange) {
                ++teamStatsFor(result, pending->sourceTeamId).passesReceiverOutOfRange;
            }
            const double receiverArrivalSecond = static_cast<double>(state.currentSecond)
                + movementDurationSeconds(
                    target->position,
                    state.ball.position,
                    target->effectivePace,
                    0.0,
                    4.0);
            const double defenderDistanceToArrival = arrivalDefender != nullptr
                ? PitchGeometry::distance(arrivalDefender->position, state.ball.position)
                : 100.0;
            const double defenderDistanceToReceiver = arrivalDefender != nullptr
                ? PitchGeometry::distance(arrivalDefender->position, target->position)
                : 100.0;
            const double defenderDistanceToLane = arrivalDefender != nullptr
                ? distancePointToSegment(
                    arrivalDefender->position,
                    trajectory.start,
                    trajectory.actualTarget)
                : 100.0;
            const double defenderArrivalSecond = arrivalDefender != nullptr
                ? static_cast<double>(state.currentSecond)
                    + movementDurationSeconds(
                        arrivalDefender->position,
                        state.ball.position,
                        arrivalDefender->effectivePace,
                        0.0,
                        4.0)
                : 100.0;
            const double defenderLaneArrivalSecond = arrivalDefender != nullptr
                ? static_cast<double>(state.currentSecond)
                    + (defenderDistanceToLane / std::max(arrivalDefender->effectivePace, 0.5))
                : 100.0;
            const PassResolutionFlowResult passFlow = PassResolutionFlow{}.resolve(
                PassResolutionFlowRequest{
                    PassResolutionContext{
                        pending->actionType,
                        trajectory.start,
                        trajectory.intendedTarget,
                        trajectory.actualTarget,
                        PitchGeometry::distance(trajectory.start, trajectory.actualTarget),
                        pending->executionQuality,
                        pending->pressure,
                        trajectory.arrivalSecond,
                        receiverDistanceToArrival,
                        controlRange,
                        receiverArrivalSecond,
                        arrivalDefender != nullptr,
                        defenderDistanceToArrival,
                        defenderDistanceToReceiver,
                        defenderDistanceToLane,
                        defenderArrivalSecond,
                        defenderLaneArrivalSecond,
                        arrivalDefender != nullptr
                            ? intentPressureContribution(arrivalDefender->currentIntent.type)
                            : 0.0,
                        stepSeed(
                            baseSeed,
                            state,
                            static_cast<std::uint64_t>(target->playerId)
                                ^ (static_cast<std::uint64_t>(
                                    arrivalDefender != nullptr ? arrivalDefender->playerId : 0) << 32)
                                ^ 0x9a55ULL)
                    },
                    target->playerId,
                    target->teamId,
                    arrivalDefender != nullptr ? arrivalDefender->playerId : 0,
                    arrivalDefender != nullptr ? arrivalDefender->teamId : 0
                });
            const PassResolutionResult& passResolution = passFlow.resolution;

            if (passResolution.outcome == PassResolutionOutcome::DefenderIntercept
                && arrivalDefender != nullptr) {
                recordDefenderPassWin(
                    state,
                    result,
                    input,
                    *pending,
                    *arrivalDefender,
                    state.ball.position,
                    assistTracker);
                pending = std::nullopt;
                return SimulationStepResult{ elapsedToArrival };
            }

            if (passResolution.outcome == PassResolutionOutcome::DeflectedLoose
                || passResolution.outcome == PassResolutionOutcome::MisplacedLoose
                || passResolution.outcome == PassResolutionOutcome::OutOfPlay) {
                recordLoosePass(
                    state,
                    result,
                    input,
                    *pending,
                    state.ball.position,
                    assistTracker);
                pending = std::nullopt;
                return SimulationStepResult{ elapsedToArrival };
            }

            setControlledBy(
                state,
                target->playerId,
                target->teamId,
                state.ball.position);
            if (target->teamId == pending->sourceTeamId) {
                ++teamStatsFor(result, pending->sourceTeamId).passesCompleted;
                ++playerStatsFor(
                    result,
                    pending->sourcePlayerId,
                    pending->sourceTeamId).passesCompleted;
                rememberCompletedPass(assistTracker, state, *pending, target->playerId);
            } else {
                clearAssist(assistTracker);
            }
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToArrival };
        }

        if (pending && pending->isShot) {
            const ShotBlockResult block = ShotBlockResolver{}.resolve(ShotBlockRequest{
                trajectory,
                pending->shotContext,
                pending->shotQuality,
                pending->shotExecution,
                shotBlockersFor(input, *defendingTeam),
                stepSeed(baseSeed, state, pending->sourcePlayerId ^ 0xb10c5ULL)
            });

            if (block.blocked) {
                const double blockDistance =
                    PitchGeometry::distance(trajectory.start, block.contactPoint);
                const double blockSecond = trajectory.startSecond
                    + (blockDistance / std::max(trajectory.speedMetersPerSecond, 1.0));
                state.ball.controlState = BallControlState::InFlight;
                state.ball.carrierPlayerId = 0;
                state.ball.carrierTeamId = 0;
                state.ball.position = block.contactPoint;
                state.possession.teamInPossession = 0;
                state.possession.lastPossessionTeamId = block.blockerTeamId;
                state.possession.ballCarrierId = 0;
                state.ball.trajectory = ReboundTrajectoryBuilder{}.build(ReboundTrajectoryRequest{
                    block.contactPoint,
                    trajectory,
                    pending->shotExecution,
                    ShotOutcomeKind::Blocked,
                    50.0,
                    block.deflectionStrength,
                    blockSecond,
                    stepSeed(baseSeed, state, block.blockerPlayerId ^ 0xdef1ec7ULL)
                });
                appendTrace(
                    result,
                    input.options.detail,
                    state,
                    MatchTraceKind::Interception,
                    block.blockerTeamId,
                    block.blockerPlayerId,
                    pending->sourcePlayerId,
                    block.contactPoint,
                    state.ball.trajectory->actualTarget);
                pending = pendingUncontrolledBall(
                    PendingBallKind::BlockedDeflection,
                    block.blockerTeamId,
                    block.blockerPlayerId,
                    BallTrajectoryType::Deflection);
                clearAssist(assistTracker);
                return SimulationStepResult{
                    clampElapsedSeconds(blockSecond - static_cast<double>(state.currentSecond))
                };
            }

            return processShotAtGoal(
                state,
                result,
                input,
                pending,
                trajectory,
                *defendingTeam,
                baseSeed,
                assistTracker);
        }

        const std::vector<PlayerSimState> interceptionDefenders =
            interceptionDefendersFor(input, *defendingTeam, pending, trajectory);
        const InterceptionResolverResult interception = interceptionResolver.resolve(
            InterceptionResolverRequest{
                trajectory,
                interceptionDefenders,
                7,
                pending ? interceptionReactionWindowFor(pending->actionType) : 0.25
            });
        const std::optional<InterceptionCandidate> reachableCandidate =
            pending
                ? bestReachableInterceptionCandidate(
                    state,
                    input,
                    trajectory,
                    interception,
                    interceptionQualityThresholdFor(pending->actionType))
                : std::nullopt;

        if (pending && reachableCandidate) {
            const InterceptionCandidate candidate = *reachableCandidate;
            const PlayerSimState* defender = findPlayerState(state, candidate.playerId);
            const PlayerSimState* attacker =
                pending->targetPlayerId != 0
                    ? findPlayerState(state, pending->targetPlayerId)
                    : findPlayerState(state, pending->sourcePlayerId);

            if (defender != nullptr && attacker != nullptr) {
                ContestResolverRequest request;
                request.type = contestTypeFor(*pending, trajectory);
                request.contestPoint = candidate.interceptionPoint;
                request.interceptionCandidate = candidate;
                request.ballArrivalSecond = candidate.ballArrivalSecond;
                request.pressure = std::max(35.0, pending->pressure);
                request.executionQuality = pending->executionQuality;
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
                        clearAssist(assistTracker);
                        if (isPassLike(pending->actionType)) {
                            ++teamStatsFor(result, pending->sourceTeamId).passesIntercepted;
                        }
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
                    return SimulationStepResult{
                        clampElapsedSeconds(
                            candidate.ballArrivalSecond - static_cast<double>(state.currentSecond))
                    };
                }

                if (contest.ballDeflected) {
                    if (isPassLike(pending->actionType)) {
                        ++teamStatsFor(result, pending->sourceTeamId).passesDeflected;
                    }
                    state.ball.controlState = BallControlState::InFlight;
                    state.ball.carrierPlayerId = 0;
                    state.ball.carrierTeamId = 0;
                    state.ball.position = candidate.interceptionPoint;
                    state.possession.teamInPossession = 0;
                    state.possession.lastPossessionTeamId = candidate.teamId;
                    state.possession.ballCarrierId = 0;
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
                    pending = pendingUncontrolledBall(
                        PendingBallKind::LooseDeflection,
                        candidate.teamId,
                        candidate.playerId,
                        BallTrajectoryType::Deflection);
                    clearAssist(assistTracker);
                    return SimulationStepResult{
                        clampElapsedSeconds(
                            candidate.ballArrivalSecond - static_cast<double>(state.currentSecond))
                    };
                }

                if (contest.ballBecomesLoose
                    || contest.ballOutcome == ContestBallOutcome::BallLoose) {
                    if (isPassLike(pending->actionType)) {
                        ++teamStatsFor(result, pending->sourceTeamId).passesLoose;
                    }
                    setLooseBall(state, candidate.interceptionPoint);
                    clearAssist(assistTracker);
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
                    return SimulationStepResult{
                        clampElapsedSeconds(
                            candidate.ballArrivalSecond - static_cast<double>(state.currentSecond))
                    };
                }

                if (contest.ballOutcome == ContestBallOutcome::ShotContinues) {
                    return processShotAtGoal(
                        state,
                        result,
                        input,
                        pending,
                        trajectory,
                        *defendingTeam,
                        baseSeed,
                        assistTracker);
                }
            }
        }

        state.ball.position = PitchGeometry::clampToPitch(trajectory.actualTarget);
        if (pending && pending->isShot) {
            return processShotAtGoal(
                state,
                result,
                input,
                pending,
                trajectory,
                *defendingTeam,
                baseSeed,
                assistTracker);
        }

        setLooseBall(state, state.ball.position);
        clearAssist(assistTracker);
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
        return SimulationStepResult{ elapsedToArrival };
    }

    SimulationStepResult processLooseBall(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        AssistTracker& assistTracker) {
        std::vector<LooseBallRecoveryCandidate> candidates;
        candidates.reserve(state.homeTeam.players.size() + state.awayTeam.players.size());
        for (PlayerSimState& player : state.homeTeam.players) {
            candidates.push_back(LooseBallRecoveryCandidate{
                player.playerId,
                player.teamId,
                player.position,
                player.currentIntent,
                player.effectivePace,
                player.effectiveAcceleration,
                player.baseOverall
            });
        }
        for (PlayerSimState& player : state.awayTeam.players) {
            candidates.push_back(LooseBallRecoveryCandidate{
                player.playerId,
                player.teamId,
                player.position,
                player.currentIntent,
                player.effectivePace,
                player.effectiveAcceleration,
                player.baseOverall
            });
        }

        const LooseBallRecoveryResult recovery = LooseBallRecoveryModel{}.resolve(
            LooseBallRecoveryContext{
                state.ball.position,
                state.possession.lastPossessionTeamId,
                candidates
            });
        if (!recovery.controlled) {
            if (PlayerSimState* pursuingPlayer = findPlayerState(state, recovery.playerId)) {
                pursuingPlayer->targetPosition = state.ball.position;
                pursuingPlayer->currentIntent = PlayerIntent{
                    PlayerIntentType::RecoverLooseBall,
                    state.ball.position,
                    0,
                    1.0
                };
            }
            return SimulationStepResult{ recovery.elapsedSeconds };
        }

        PlayerSimState* best = findPlayerState(state, recovery.playerId);
        if (best == nullptr) {
            return SimulationStepResult{ recovery.elapsedSeconds };
        }

        const TeamId previousTeam = state.possession.lastPossessionTeamId;
        setControlledBy(state, best->playerId, best->teamId, state.ball.position);
        if (previousTeam != 0 && previousTeam != best->teamId) {
            clearAssist(assistTracker);
        }
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
        return SimulationStepResult{ recovery.elapsedSeconds };
    }

    SimulationStepResult processOutOfPlay(MatchSimulationState& state) {
        setLooseBall(state, pitchCenter());
        return SimulationStepResult{ 1.0 };
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

    void finalizePlayerMinutes(MatchEngineResult& result, int simulatedSeconds) {
        const int minutes = std::clamp(
            static_cast<int>(std::ceil(static_cast<double>(simulatedSeconds) / 60.0)),
            0,
            90);
        for (MatchPlayerSimulationStats& stats : result.playerStats) {
            stats.minutesPlayed = std::max(stats.minutesPlayed, minutes);
        }
    }

    int goalsForTeam(const MatchEngineResult& result, TeamId teamId) {
        if (result.homeStats.teamId == teamId) {
            return result.homeStats.goals;
        }
        if (result.awayStats.teamId == teamId) {
            return result.awayStats.goals;
        }
        return 0;
    }

    int goalsAgainstTeam(const MatchEngineResult& result, TeamId teamId) {
        if (result.homeStats.teamId == teamId) {
            return result.awayStats.goals;
        }
        if (result.awayStats.teamId == teamId) {
            return result.homeStats.goals;
        }
        return 0;
    }

    const MatchTeamSnapshot* teamSnapshotForPlayerReport(
        const MatchEngineInput& input,
        const MatchPlayerSimulationStats& stats) {
        if (stats.teamId == input.homeTeam.teamId) {
            return &input.homeTeam;
        }
        if (stats.teamId == input.awayTeam.teamId) {
            return &input.awayTeam;
        }
        return nullptr;
    }

    void finalizePlayerRatings(MatchEngineResult& result, const MatchEngineInput& input) {
        const PlayerRatingModel ratingModel;
        for (MatchPlayerSimulationStats& stats : result.playerStats) {
            const MatchTeamSnapshot* snapshot = teamSnapshotForPlayerReport(input, stats);
            const FormationSlotRole role =
                snapshot != nullptr ? assignedRoleFor(snapshot->teamSheet, stats.playerId) : FormationSlotRole::Unknown;
            stats.rating = ratingModel.calculate(PlayerRatingContext{
                stats,
                role,
                goalsForTeam(result, stats.teamId),
                goalsAgainstTeam(result, stats.teamId)
            });
        }
    }
}

MatchEngineResult CoordinateMatchSimulator::run(const MatchEngineInput& input) const {
    MatchEngineResult result;
    result.homeStats.teamId = input.homeTeam.teamId;
    result.awayStats.teamId = input.awayTeam.teamId;

    MatchSimulationState state = initializeState(input);
    addInitialPlayerStats(result, state);

    const std::uint64_t baseSeed = baseSeedFor(input);
    const double deltaSeconds = deltaSecondsFor(input);

    TeamShapeModel shapeModel;
    PlayerIntentResolver intentResolver;
    MovementResolver movementResolver;
    BallCarrierDecisionModel ballCarrierDecisionModel;
    ActionSelector actionSelector;
    BallTrajectoryBuilder trajectoryBuilder;
    InterceptionResolver interceptionResolver;
    ContestResolver contestResolver;

    std::optional<ContestResolverResult> lastContestResult;
    std::optional<PendingBallAction> pending;
    AssistTracker assistTracker;

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

    int safetyCounter = 0;
    while (state.currentSecond < RegulationMatchSeconds
        && safetyCounter < MaxSafetyActions) {
        ++safetyCounter;
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

        const PlayerId movementSkipPlayerId =
            state.ball.controlState == BallControlState::Controlled
                ? state.ball.carrierPlayerId
                : 0;

        moveTeamPlayers(
            movementResolver,
            state.homeTeam,
            homeIntents,
            deltaSeconds,
            movementSkipPlayerId);
        moveTeamPlayers(
            movementResolver,
            state.awayTeam,
            awayIntents,
            deltaSeconds,
            movementSkipPlayerId);

        SimulationStepResult step;

        if (state.ball.controlState == BallControlState::Controlled) {
            const TeamShapeContext& carrierContext =
                state.ball.carrierTeamId == state.homeTeam.teamId
                    ? homeShapeContext
                    : awayShapeContext;
            step = executeControlledAction(
                state,
                result,
                input,
                carrierContext,
                trajectoryBuilder,
                ballCarrierDecisionModel,
                actionSelector,
                pending,
                baseSeed,
                deltaSeconds);
        } else if (state.ball.controlState == BallControlState::InFlight) {
            step = processInFlightBall(
                state,
                result,
                input,
                trajectoryBuilder,
                interceptionResolver,
                contestResolver,
                lastContestResult,
                pending,
                baseSeed,
                assistTracker);
        } else if (state.ball.controlState == BallControlState::Loose) {
            step = processLooseBall(state, result, input, assistTracker);
        } else if (state.ball.controlState == BallControlState::OutOfPlay) {
            step = processOutOfPlay(state);
        }

        const double elapsedSeconds = clampElapsedSeconds(
            step.elapsedSeconds,
            MinimumStepSeconds,
            RestartAfterGoalSeconds + MaximumNormalStepSeconds);
        if (state.possession.teamInPossession == state.homeTeam.teamId) {
            state.homeTeam.possessionShareAccumulator += elapsedSeconds;
        } else if (state.possession.teamInPossession == state.awayTeam.teamId) {
            state.awayTeam.possessionShareAccumulator += elapsedSeconds;
        }

        updateMeaningfulProgression(state);
        state.currentSecond = std::min(
            RegulationMatchSeconds,
            state.currentSecond + static_cast<int>(std::ceil(elapsedSeconds)));
    }

    result.simulatedSeconds = state.currentSecond;
    finalizePlayerMinutes(result, state.currentSecond);
    finalizePossessionShare(result, state);
    finalizePlayerRatings(result, input);
    result.report = MatchEngineReportAdapter{}.buildReport(input, result);
    return result;
}
