#include"fm/match_engine/simulation/CoordinateMatchSimulator.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/ActionCandidateGenerator.h"
#include"fm/match_engine/decision/ActionSelector.h"
#include"fm/match_engine/ball/BallTrajectoryBuilder.h"
#include"fm/match_engine/contest/ContestResolver.h"
#include"fm/match_engine/contest/InterceptionResolver.h"
#include"fm/match_engine/reporting/MatchEngineReportAdapter.h"
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
    constexpr double LooseBallControlRangeMeters = 1.5;
    constexpr int RegulationMatchSeconds = 90 * 60;
    constexpr int MaxSafetyActions = 8000;
    constexpr double MinimumStepSeconds = 0.5;
    constexpr double MaximumNormalStepSeconds = 15.0;
    constexpr double RestartAfterGoalSeconds = 20.0;

    struct SimulationStepResult {
        double elapsedSeconds = 1.0;
    };

    struct PendingBallAction {
        TeamId sourceTeamId = 0;
        PlayerId sourcePlayerId = 0;
        PlayerId targetPlayerId = 0;
        BallCarrierActionType actionType = BallCarrierActionType::Hold;
        BallTrajectoryType trajectoryType = BallTrajectoryType::GroundPass;
        double executionQuality = 70.0;
        double pressure = 0.0;
        bool isShot = false;
        double shotXG = 0.0;
    };

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
            || type == PlayerIntentType::MarkOpponent;
    }

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
                if (!isInterceptionIntent(player.currentIntent.type)
                    && std::min(distanceToStart, distanceToEnd) > 8.0) {
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
        const TeamId previousTeam = state.possession.teamInPossession;
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
        if (previousTeam != teamId) {
            state.possession.possessionStartSecond = state.currentSecond;
            state.possession.actionDepth = 0;
            state.possession.isTransition = true;
        } else {
            state.possession.isTransition = false;
        }
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

    double openPlayXGFor(
        PitchPoint shotLocation,
        AttackingDirection direction,
        double pressure) {
        return ShotQualityModel::calculateOpenPlayXG(shotLocation, direction, pressure);
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

    double goalProbabilityForShot(
        const MatchEngineInput& input,
        const PendingBallAction& pending,
        const PlayerSimState* goalkeeper) {
        const MatchPlayerSnapshot* shooter = findSnapshotForPlayer(input, pending.sourcePlayerId);
        const double shooterModifier = shooter == nullptr
            ? 1.0
            : clampDouble(
                0.98
                    + (clampedAttribute(shooter->attributes.technical.shooting) - 60.0) / 260.0
                    + (clampedAttribute(shooter->attributes.mental.composure) - 60.0) / 330.0,
                0.85,
                1.18);
        const double goalkeeperStrength = goalkeeperStrengthFor(input, goalkeeper);
        const double goalkeeperModifier = clampDouble(1.02 - ((goalkeeperStrength - 60.0) / 230.0), 0.78, 1.16);
        const double pressureModifier = clampDouble(1.0 - (pending.pressure / 650.0), 0.88, 1.0);
        return clampDouble(pending.shotXG * shooterModifier * goalkeeperModifier * pressureModifier, 0.008, 0.42);
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
        const ActionCandidateGenerator& generator,
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

        const std::vector<ActionCandidate> candidates = generator.generate(
            ActionCandidateGenerationRequest{
                carrierSnapshot,
                snapshotForTeam(input, opponentState->teamId),
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
            return SimulationStepResult{ 1.0 };
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
            const double holdSeconds = 1.0
                + matchEngineDeterministicUnitInterval(
                    stepSeed(baseSeed, state, carrier->playerId ^ 0x681dULL)) * 2.0;
            return SimulationStepResult{ clampElapsedSeconds(holdSeconds, 1.0, 3.0) };
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
            return SimulationStepResult{ elapsedSeconds };
        }

        const BallTrajectoryType trajectoryType = trajectoryTypeFor(actionType);
        const double executionQuality = executionQualityFor(playerSnapshot, actionType);
        const BallTrajectoryBuildResult trajectory = trajectoryBuilder.build(BallTrajectoryBuildRequest{
            state.ball.position,
            action.intendedTarget,
            trajectoryType,
            static_cast<double>(state.currentSecond),
            executionQuality,
            action.pressurePenalty,
            stepSeed(
                baseSeed,
                state,
                static_cast<std::uint64_t>(carrier->playerId)
                    ^ (static_cast<std::uint64_t>(actionType) << 32))
        });

        double shotXG = 0.0;
        if (isPassLike(actionType)) {
            ++teamStatsFor(result, carrier->teamId).passesAttempted;
            ++playerStatsFor(result, carrier->playerId, carrier->teamId).passesAttempted;
        } else if (actionType == BallCarrierActionType::Shoot) {
            shotXG = openPlayXGFor(
                state.ball.position,
                carrierShapeContext.attackingDirection,
                action.pressurePenalty);
            ++teamStatsFor(result, carrier->teamId).shots;
            teamStatsFor(result, carrier->teamId).expectedGoals += shotXG;
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
            executionQuality,
            action.pressurePenalty,
            actionType == BallCarrierActionType::Shoot,
            shotXG
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
        return SimulationStepResult{ trajectoryElapsedSeconds(trajectory.trajectory) };
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

    SimulationStepResult processShotAtGoal(
        MatchSimulationState& state,
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const BallTrajectoryBuilder& trajectoryBuilder,
        const ContestResolver& contestResolver,
        std::optional<ContestResolverResult>& lastContestResult,
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
        const bool onTarget = shotCrossesGoalMouth(trajectory, direction);
        if (!onTarget) {
            setLooseBall(state, trajectory.actualTarget);
            clearAssist(assistTracker);
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
            return SimulationStepResult{ elapsedToShot };
        }

        ++teamStatsFor(result, pending->sourceTeamId).shotsOnTarget;

        const PlayerSimState* goalkeeper = findGoalkeeperOrNearestOwnGoal(input, defendingTeam);
        const PlayerSimState* shooter = findPlayerState(state, pending->sourcePlayerId);
        if (goalkeeper == nullptr || shooter == nullptr) {
            const double goalProbability = goalProbabilityForShot(input, *pending, nullptr);
            const bool converted =
                matchEngineDeterministicUnitInterval(
                    stepSeed(baseSeed, state, pending->sourcePlayerId ^ 0x9010ULL)) < goalProbability;
            if (converted) {
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
            setLooseBall(state, trajectory.actualTarget);
            clearAssist(assistTracker);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToShot };
        }

        const PitchPoint savePoint = trajectory.actualTarget;
        ContestResolverRequest request;
        request.type = ContestType::GoalkeeperSave;
        request.contestPoint = savePoint;
        request.ballArrivalSecond = trajectory.arrivalSecond;
        request.pressure = std::max(35.0, pending->pressure);
        request.executionQuality = pending->executionQuality;
        request.seed = stepSeed(
            baseSeed,
            state,
            static_cast<std::uint64_t>(pending->sourcePlayerId)
                ^ (static_cast<std::uint64_t>(goalkeeper->playerId) << 32)
                ^ 0x51a7e5a9ULL);
        request.participants.push_back(participantFor(
            *goalkeeper,
            findSnapshotForPlayer(input, goalkeeper->playerId),
            ContestSide::Defending,
            0.0,
            0.0));
        request.participants.push_back(participantFor(
            *shooter,
            findSnapshotForPlayer(input, shooter->playerId),
            ContestSide::Attacking,
            trajectory.arrivalSecond,
            (pending->executionQuality - 50.0) * 0.08));

        const ContestResolverResult contest = contestResolver.resolve(request);
        lastContestResult = contest;
        const double goalProbability = goalProbabilityForShot(input, *pending, goalkeeper);
        const bool converted =
            matchEngineDeterministicUnitInterval(
                stepSeed(
                    baseSeed,
                    state,
                    static_cast<std::uint64_t>(pending->sourcePlayerId)
                        ^ (static_cast<std::uint64_t>(goalkeeper->playerId) << 32)
                        ^ 0x9011ULL)) < goalProbability;

        if (contest.ballOutcome == ContestBallOutcome::KeeperControls
            || (contest.cleanController
                && contest.cleanController->playerId == goalkeeper->playerId)) {
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
            return SimulationStepResult{ elapsedToShot };
        }

        if (contest.ballOutcome == ContestBallOutcome::BallDeflected || contest.ballDeflected) {
            state.ball.controlState = BallControlState::InFlight;
            state.ball.carrierPlayerId = 0;
            state.ball.carrierTeamId = 0;
            state.ball.position = savePoint;
            state.ball.trajectory = trajectoryBuilder.buildDeflectedTrajectory(
                DeflectedBallTrajectoryRequest{
                    savePoint,
                    trajectory.start,
                    trajectory.actualTarget,
                    0.5,
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
            pending = PendingBallAction{
                goalkeeper->teamId,
                goalkeeper->playerId,
                0,
                BallCarrierActionType::Hold,
                BallTrajectoryType::Deflection,
                50.0,
                0.0,
                false
            };
            clearAssist(assistTracker);
            return SimulationStepResult{ elapsedToShot };
        }

        if (contest.ballOutcome == ContestBallOutcome::BallLoose || contest.ballBecomesLoose) {
            setLooseBall(state, savePoint);
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
            appendTrace(
                result,
                input.options.detail,
                state,
                MatchTraceKind::LooseBall,
                goalkeeper->teamId,
                goalkeeper->playerId,
                pending->sourcePlayerId,
                savePoint,
                savePoint);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToShot };
        }

        if (converted) {
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
        TeamSimState* defendingTeam = opponentTeam(state, state.possession.teamInPossession);
        if (defendingTeam == nullptr) {
            setLooseBall(state, trajectory.actualTarget);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToArrival };
        }

        if (pending) {
            moveReceiverTowardPass(state, input, *pending, trajectory);
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
                        candidate.teamId,
                        candidate.playerId,
                        0,
                        pending->actionType,
                        BallTrajectoryType::Deflection,
                        50.0,
                        0.0,
                        false
                    };
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
                        trajectoryBuilder,
                        contestResolver,
                        lastContestResult,
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
                trajectoryBuilder,
                contestResolver,
                lastContestResult,
                pending,
                trajectory,
                *defendingTeam,
                baseSeed,
                assistTracker);
        }

        if (pending
            && !pending->isShot
            && pending->targetPlayerId != 0) {
            PlayerSimState* target = findPlayerState(state, pending->targetPlayerId);
            const MatchPlayerSnapshot* targetSnapshot =
                target != nullptr ? findSnapshotForPlayer(input, target->playerId) : nullptr;
            const double controlRange = receptionControlRange(
                pending->actionType,
                pending->executionQuality,
                targetSnapshot,
                pending->pressure);
            if (target != nullptr
                && PitchGeometry::distance(target->position, state.ball.position)
                    <= controlRange) {
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
                    rememberCompletedPass(assistTracker, state, *pending, target->playerId);
                } else {
                    clearAssist(assistTracker);
                }
                pending = std::nullopt;
                return SimulationStepResult{ elapsedToArrival };
            }

            if (isPassLike(pending->actionType)) {
                ++teamStatsFor(result, pending->sourceTeamId).passesReceiverOutOfRange;
            }
        }

        if (pending && isPassLike(pending->actionType) && pending->targetPlayerId == 0) {
            ++teamStatsFor(result, pending->sourceTeamId).passesLoose;
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
            return SimulationStepResult{ best != nullptr
                ? movementDurationSeconds(
                    best->position,
                    state.ball.position,
                    best->effectivePace,
                    0.5,
                    4.0)
                : 1.0 };
        }

        const double raceSeconds =
            movementDurationSeconds(best->position, state.ball.position, best->effectivePace, 0.5, 4.0);
        const TeamId previousTeam = state.possession.teamInPossession;
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
        return SimulationStepResult{ raceSeconds };
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

    bool isDefensiveRatingRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
        case FormationSlotRole::DefensiveMidfielder:
            return true;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
        case FormationSlotRole::Striker:
            return false;
        }

        return false;
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
        for (MatchPlayerSimulationStats& stats : result.playerStats) {
            const MatchTeamSnapshot* snapshot = teamSnapshotForPlayerReport(input, stats);
            const FormationSlotRole role =
                snapshot != nullptr ? assignedRoleFor(snapshot->teamSheet, stats.playerId) : FormationSlotRole::Unknown;
            const int goalsFor = goalsForTeam(result, stats.teamId);
            const int goalsAgainst = goalsAgainstTeam(result, stats.teamId);

            double rating = 6.0
                + static_cast<double>(stats.goals) * 0.75
                + static_cast<double>(stats.assists) * 0.45
                + static_cast<double>(stats.shots) * 0.01
                + static_cast<double>(stats.interceptions) * 0.05
                + std::min(0.25, static_cast<double>(stats.passesCompleted) * 0.001)
                - static_cast<double>(stats.yellowCards) * 0.15
                - static_cast<double>(stats.redCards) * 1.0;

            if (goalsFor > goalsAgainst) {
                rating += 0.15;
            } else if (goalsFor < goalsAgainst) {
                rating -= 0.15;
            }

            if (isDefensiveRatingRole(role)) {
                if (goalsAgainst == 0) {
                    rating += 0.18;
                } else {
                    rating -= std::min(0.60, static_cast<double>(goalsAgainst) * 0.10);
                }
            }

            const bool exceptional =
                stats.goals >= 3
                || (stats.goals >= 2 && stats.assists >= 1)
                || stats.assists >= 3;
            stats.rating = clampDouble(rating, 4.5, exceptional ? 10.0 : 9.8);
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
    ActionCandidateGenerator actionGenerator;
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
                actionGenerator,
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

        ++state.possession.actionDepth;
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
