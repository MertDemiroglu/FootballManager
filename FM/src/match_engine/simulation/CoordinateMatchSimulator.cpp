#include"fm/match_engine/simulation/CoordinateMatchSimulator.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/ActionSelector.h"
#include"fm/match_engine/decision/BallCarrierDecisionModel.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"
#include"fm/match_engine/decision/PhaseDecisionContext.h"
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
#include"fm/match_engine/contest/DuelResolver.h"
#include"fm/match_engine/contest/InterceptionResolver.h"
#include"fm/match_engine/contest/PressureContext.h"
#include"fm/match_engine/reporting/MatchEngineReportAdapter.h"
#include"fm/match_engine/reporting/PlayerRatingModel.h"
#include"fm/match_engine/movement/MovementResolver.h"
#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/phase/MatchPhaseModel.h"
#include"fm/match_engine/decision/PlayerIntentResolver.h"
#include"fm/match_engine/ball/ShotQualityModel.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/offball/OffBallEventLifecycle.h"
#include"fm/match_engine/offball/OffBallSupportModel.h"
#include"fm/match_engine/offball/RestDefenseModel.h"

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
        PlayerId assistCandidatePlayerId = 0;
        bool shotFromFinalBall = false;
        bool shotFromReceiverCarry = false;
        bool shotFromRebound = false;
        bool shotFromTurnover = false;
        BallCarrierActionType chanceSourceActionType = BallCarrierActionType::Hold;
        double carryAfterReceiveMeters = 0.0;
        int touchesAfterReceive = 0;
        int defendersBeatenAfterReceive = 0;
        MatchTeamPhase sourcePhase = MatchTeamPhase::BuildUp;
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
        ShotQualityResult shotQuality = ShotQualityResult{},
        PlayerId assistCandidatePlayerId = 0,
        bool shotFromFinalBall = false,
        bool shotFromReceiverCarry = false,
        bool shotFromRebound = false,
        bool shotFromTurnover = false,
        BallCarrierActionType chanceSourceActionType = BallCarrierActionType::Hold,
        double carryAfterReceiveMeters = 0.0,
        int touchesAfterReceive = 0,
        int defendersBeatenAfterReceive = 0,
        MatchTeamPhase sourcePhase = MatchTeamPhase::BuildUp) {
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
        pending.assistCandidatePlayerId = assistCandidatePlayerId;
        pending.shotFromFinalBall = shotFromFinalBall;
        pending.shotFromReceiverCarry = shotFromReceiverCarry;
        pending.shotFromRebound = shotFromRebound;
        pending.shotFromTurnover = shotFromTurnover;
        pending.chanceSourceActionType = chanceSourceActionType;
        pending.carryAfterReceiveMeters = carryAfterReceiveMeters;
        pending.touchesAfterReceive = touchesAfterReceive;
        pending.defendersBeatenAfterReceive = defendersBeatenAfterReceive;
        pending.sourcePhase = sourcePhase;
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

    struct ChanceCreationSource {
        PlayerId passerPlayerId = 0;
        PlayerId receiverPlayerId = 0;
        TeamId teamId = 0;
        BallCarrierActionType actionType = BallCarrierActionType::Hold;
        PitchPoint passTargetPoint;
        PitchPoint receivePoint;
        int second = 0;
        int possessionActionDepth = 0;
        bool chanceCreatingFinalBall = false;
        double estimatedPreShotXGAtReceive = 0.0;
        bool followedByControlledCarry = false;
        int controlledCarryCount = 0;
        double controlledCarryDistance = 0.0;
        int defendersBeatenAfterReceive = 0;
    };

    struct ChanceCreationTracker {
        std::optional<ChanceCreationSource> active;
        bool reboundSequenceActive = false;
        bool turnoverSequenceActive = false;
    };

    bool isPassLike(BallCarrierActionType type);
    bool isCarryLike(BallCarrierActionType type);
    bool isFinalBall(BallCarrierActionType type);
    double forwardMetersForDirection(PitchPoint from, PitchPoint to, AttackingDirection direction);
    int diagnosticActionTypeIndex(BallCarrierActionType type);
    bool isRecycleAction(
        BallCarrierActionType type,
        PitchPoint from,
        PitchPoint to,
        AttackingDirection direction);

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

    struct ChanceCreationProfile {
        double finalBallMinimumGoalDistance = 4.0;
        double finalBallMaximumGoalDistance = 18.0;
        double finalBallMinimumForwardMeters = 5.0;
        int maximumSecondsAfterReceive = 18;
        int maximumActionsAfterReceive = 5;
        int maximumControlledCarries = 3;
        double maximumControlledCarryDistance = 22.0;
    };

    void clearChanceSource(ChanceCreationTracker& tracker) {
        tracker.active = std::nullopt;
    }

    void clearChanceChain(ChanceCreationTracker& tracker) {
        tracker = ChanceCreationTracker{};
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

    MatchTeamPhaseDiagnostic& teamPhaseDiagnosticFor(MatchEngineResult& result, TeamId teamId) {
        for (MatchTeamPhaseDiagnostic& diagnostic : result.phaseDiagnostics.teamDiagnostics) {
            if (diagnostic.teamId == teamId) {
                return diagnostic;
            }
        }

        MatchTeamPhaseDiagnostic diagnostic;
        diagnostic.teamId = teamId;
        result.phaseDiagnostics.teamDiagnostics.push_back(diagnostic);
        return result.phaseDiagnostics.teamDiagnostics.back();
    }

    void recordPhaseEntry(MatchEngineResult& result, TeamSimState& team, MatchTeamPhase phase) {
        const int index = matchTeamPhaseIndex(phase);
        ++result.phaseDiagnostics.phaseEntries[index];
        ++teamPhaseDiagnosticFor(result, team.teamId).phaseEntries[index];
        if (phase == MatchTeamPhase::CounterAttack) {
            ++result.phaseDiagnostics.counterEntries;
            ++result.phaseDiagnostics.validCounterEntries;
        } else if (phase == MatchTeamPhase::DefensiveTransition) {
            ++result.phaseDiagnostics.defensiveTransitionEntries;
        } else if (phase == MatchTeamPhase::SettledDefense) {
            ++result.phaseDiagnostics.settledDefenseEntries;
        }
    }

    bool containsText(const std::string& value, const char* text) {
        return value.find(text) != std::string::npos;
    }

    void recordCounterRejection(MatchEngineResult& result, CounterRejectionReason reason) {
        switch (reason) {
        case CounterRejectionReason::NoCleanRegain:
            ++result.phaseDiagnostics.counterRejectedNoCleanRegain;
            break;
        case CounterRejectionReason::NoForwardLane:
            ++result.phaseDiagnostics.counterRejectedNoForwardLane;
            break;
        case CounterRejectionReason::NoRunner:
            ++result.phaseDiagnostics.counterRejectedNoRunner;
            break;
        case CounterRejectionReason::OpponentRecovered:
            ++result.phaseDiagnostics.counterRejectedOpponentRecovered;
            break;
        case CounterRejectionReason::SettledPossession:
            ++result.phaseDiagnostics.counterRejectedSettledPossession;
            break;
        case CounterRejectionReason::None:
            break;
        }
    }

    void recordCounterExit(
        MatchEngineResult& result,
        const std::string& exitReason,
        double durationSeconds) {
        result.phaseDiagnostics.counterDurationTotalSeconds += durationSeconds;
        result.phaseDiagnostics.counterDurationSamplesSeconds.push_back(durationSeconds);
        if (containsText(exitReason, "no forward lane")) {
            ++result.phaseDiagnostics.counterExpiredNoForwardLane;
        } else if (containsText(exitReason, "defense recovered")) {
            ++result.phaseDiagnostics.counterExpiredDefenseRecovered;
        } else if (containsText(exitReason, "forced backward")) {
            ++result.phaseDiagnostics.counterExpiredForcedBackwardOrSideways;
        } else if (containsText(exitReason, "recycled")) {
            ++result.phaseDiagnostics.counterExpiredRecycledToBuildUp;
        }
    }

    void startFinalizingDiagnostic(
        MatchEngineResult& result,
        TeamSimState& team,
        int possessionActionDepth,
        bool fromBuildUp);

    void finishFinalizingDiagnostic(
        MatchEngineResult& result,
        TeamSimState& team,
        MatchTeamPhase nextPhase);

    void applyPhaseTransition(
        MatchEngineResult& result,
        TeamSimState& team,
        const PhaseTransitionResult& transition,
        int currentSecond,
        int possessionActionDepth) {
        if (transition.heldForLooseBall) {
            ++result.phaseDiagnostics.looseBallPhaseHolds;
        }
        recordCounterRejection(result, transition.counterRejection);

        if (!transition.changed || transition.phase == team.currentPhase) {
            return;
        }

        const double elapsedInPrevious = static_cast<double>(
            std::max(0, currentSecond - team.phaseEntrySecond));
        MatchTeamPhaseDiagnostic& teamDiagnostic = teamPhaseDiagnosticFor(result, team.teamId);
        teamDiagnostic.longestSinglePhaseSeconds =
            std::max(teamDiagnostic.longestSinglePhaseSeconds, elapsedInPrevious);
        ++teamDiagnostic.phaseSwitchCount;
        ++result.phaseDiagnostics.phaseSwitchCount;
        const bool enteredFinalizingFromBuildUp =
            team.currentPhase == MatchTeamPhase::BuildUp
            && transition.phase == MatchTeamPhase::FinalizingPosition;

        if (team.currentPhase == MatchTeamPhase::BuildUp
            && transition.phase == MatchTeamPhase::FinalizingPosition) {
            ++result.phaseDiagnostics.buildUpToFinalizingSwitches;
        } else if (team.currentPhase == MatchTeamPhase::FinalizingPosition
            && transition.phase == MatchTeamPhase::BuildUp) {
            ++result.phaseDiagnostics.finalizingToBuildUpSwitches;
        }
        if (transition.phase == MatchTeamPhase::CounterAttack) {
            ++result.phaseDiagnostics.anyToCounterSwitches;
        }
        if (transition.phase == MatchTeamPhase::DefensiveTransition) {
            ++result.phaseDiagnostics.anyToDefensiveTransitionSwitches;
        }
        if (team.currentPhase == MatchTeamPhase::DefensiveTransition
            && transition.phase == MatchTeamPhase::SettledDefense) {
            ++result.phaseDiagnostics.defensiveTransitionToSettledDefenseSwitches;
        }

        if (team.currentPhase == MatchTeamPhase::CounterAttack) {
            recordCounterExit(result, transition.exitReason, elapsedInPrevious);
        }
        if (team.currentPhase == MatchTeamPhase::FinalizingPosition) {
            result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingDurationSeconds +=
                elapsedInPrevious;
            finishFinalizingDiagnostic(result, team, transition.phase);
        }

        team.previousPhase = team.currentPhase;
        team.currentPhase = transition.phase;
        team.phaseEntrySecond = currentSecond;
        team.phaseEntryReason = transition.entryReason;
        team.phaseExitReason = transition.exitReason;
        recordPhaseEntry(result, team, team.currentPhase);
        if (team.currentPhase == MatchTeamPhase::FinalizingPosition) {
            startFinalizingDiagnostic(
                result,
                team,
                possessionActionDepth,
                enteredFinalizingFromBuildUp);
        }
    }

    void initializePhaseDiagnostics(MatchEngineResult& result, MatchSimulationState& state, const MatchEngineInput& input) {
        state.homeTeam.currentPhase = MatchTeamPhase::BuildUp;
        state.homeTeam.previousPhase = MatchTeamPhase::BuildUp;
        state.homeTeam.phaseEntrySecond = state.currentSecond;
        state.homeTeam.phaseEntryReason = "initial kickoff possession";

        state.awayTeam.currentPhase = MatchTeamPhase::SettledDefense;
        state.awayTeam.previousPhase = MatchTeamPhase::SettledDefense;
        state.awayTeam.phaseEntrySecond = state.currentSecond;
        state.awayTeam.phaseEntryReason = "initial out of possession shape";

        result.phaseDiagnostics.defaultFormationFourThreeThree =
            input.homeTeam.teamSheet.formation == FormationId::FourThreeThree
            && input.awayTeam.teamSheet.formation == FormationId::FourThreeThree;
        recordPhaseEntry(result, state.homeTeam, state.homeTeam.currentPhase);
        recordPhaseEntry(result, state.awayTeam, state.awayTeam.currentPhase);
    }

    void recordPhaseTime(
        MatchEngineResult& result,
        const TeamSimState& team,
        const TeamGameContext& context,
        double elapsedSeconds) {
        const int index = matchTeamPhaseIndex(team.currentPhase);
        result.phaseDiagnostics.phaseTimeSeconds[index] += elapsedSeconds;

        MatchTeamPhaseDiagnostic& teamDiagnostic = teamPhaseDiagnosticFor(result, team.teamId);
        teamDiagnostic.phaseTimeSeconds[index] += elapsedSeconds;
        teamDiagnostic.finalPhase = team.currentPhase;

        result.phaseDiagnostics.teamShapeSettledSamples += 1.0;
        if (context.teamShapeSettled) {
            result.phaseDiagnostics.teamShapeSettledCount += 1.0;
        }
        if (context.opponentShapeSettled) {
            result.phaseDiagnostics.opponentShapeSettledCount += 1.0;
        }
        if (context.restDefenseStable) {
            ++result.phaseDiagnostics.restDefenseStableCount;
        } else {
            ++result.phaseDiagnostics.restDefenseBrokenCount;
        }
        result.phaseDiagnostics.openForwardLaneTotal += context.openForwardLaneScore;
        result.phaseDiagnostics.openWideLaneLeftTotal += context.wideSpaceAvailableLeft ? 100.0 : context.openWideLaneScore;
        result.phaseDiagnostics.openWideLaneRightTotal += context.wideSpaceAvailableRight ? 100.0 : context.openWideLaneScore;
        result.phaseDiagnostics.centralSpaceAvailableTotal += context.centralSpaceAvailable ? 1.0 : 0.0;

        if (context.hasPossession) {
            if (context.ballFlank == BallFlank::Left) {
                result.phaseDiagnostics.ballFlankLeftPossessionSeconds += elapsedSeconds;
            } else if (context.ballFlank == BallFlank::Right) {
                result.phaseDiagnostics.ballFlankRightPossessionSeconds += elapsedSeconds;
            } else {
                result.phaseDiagnostics.ballFlankCenterPossessionSeconds += elapsedSeconds;
            }
        }
    }

    void finalizePhaseDiagnostics(MatchEngineResult& result, MatchSimulationState& state) {
        for (TeamSimState* team : { &state.homeTeam, &state.awayTeam }) {
            MatchTeamPhaseDiagnostic& diagnostic = teamPhaseDiagnosticFor(result, team->teamId);
            diagnostic.finalPhase = team->currentPhase;
            diagnostic.longestSinglePhaseSeconds = std::max(
                diagnostic.longestSinglePhaseSeconds,
                static_cast<double>(std::max(0, state.currentSecond - team->phaseEntrySecond)));
            if (team->currentPhase == MatchTeamPhase::CounterAttack) {
                const double duration = static_cast<double>(
                    std::max(0, state.currentSecond - team->phaseEntrySecond));
                result.phaseDiagnostics.counterDurationTotalSeconds += duration;
                result.phaseDiagnostics.counterDurationSamplesSeconds.push_back(duration);
            }
            if (team->currentPhase == MatchTeamPhase::FinalizingPosition) {
                result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingDurationSeconds +=
                    static_cast<double>(std::max(0, state.currentSecond - team->phaseEntrySecond));
                finishFinalizingDiagnostic(result, *team, team->currentPhase);
            }
        }
    }

    void recordPhaseAction(
        MatchEngineResult& result,
        MatchTeamPhase phase,
        BallCarrierActionType actionType,
        double xG = 0.0) {
        const int index = matchTeamPhaseIndex(phase);
        if (isPassLike(actionType)) {
            ++result.phaseDiagnostics.passesByPhase[index];
            if (actionType == BallCarrierActionType::ThroughBall
                || actionType == BallCarrierActionType::LowCross
                || actionType == BallCarrierActionType::HighCross
                || actionType == BallCarrierActionType::Cutback) {
                ++result.phaseDiagnostics.finalBallsByPhase[index];
                if (phase == MatchTeamPhase::FinalizingPosition) {
                    ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingFinalBalls;
                }
            }
        } else if (actionType == BallCarrierActionType::Carry) {
            ++result.phaseDiagnostics.carriesByPhase[index];
        } else if (actionType == BallCarrierActionType::Dribble
            || actionType == BallCarrierActionType::CutInside) {
            ++result.phaseDiagnostics.dribblesByPhase[index];
        } else if (actionType == BallCarrierActionType::Shoot) {
            ++result.phaseDiagnostics.shotsByPhase[index];
            result.phaseDiagnostics.xGByPhase[index] += xG;
            if (phase == MatchTeamPhase::CounterAttack) {
                ++result.phaseDiagnostics.counterShots;
                result.phaseDiagnostics.counterXG += xG;
            }
        }
    }

    void recordPhaseDecisionCandidates(
        MatchEngineResult& result,
        MatchTeamPhase phase,
        FormationSlotRole carrierRole,
        const std::vector<ActionCandidate>& candidates) {
        PhaseDecisionDiagnostics& diagnostics = result.phaseDiagnostics.phaseDecisionDiagnostics;
        const int index = matchTeamPhaseIndex(phase);
        const int carrierBucket =
            phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(carrierRole));
        for (const ActionCandidate& candidate : candidates) {
            if (isPassLike(candidate.type)) {
                ++diagnostics.passCandidatesGenerated[index];
                if (isFinalBall(candidate.type)) {
                    ++diagnostics.finalBallCandidatesGenerated[index];
                    ++diagnostics.generatedFinalBallCandidatesByCarrierRole[carrierBucket];
                }
                if (candidate.type == BallCarrierActionType::ThroughBall) {
                    ++diagnostics.generatedThroughBallCandidates;
                } else if (candidate.type == BallCarrierActionType::Cutback) {
                    ++diagnostics.generatedCutbackCandidates;
                }
            } else if (isCarryLike(candidate.type)) {
                ++diagnostics.carryCandidatesGenerated[index];
                if (candidate.type == BallCarrierActionType::CutInside) {
                    ++diagnostics.generatedCutInsideCandidates;
                }
            } else if (candidate.type == BallCarrierActionType::Shoot) {
                ++diagnostics.shotCandidatesGenerated[index];
                ++diagnostics.generatedShotCandidatesByCarrierRole[carrierBucket];
                diagnostics.shotCandidateScoreTotalByCarrierRole[carrierBucket] +=
                    candidate.finalScore;
                diagnostics.maxShotCandidateScoreByCarrierRole[carrierBucket] = std::max(
                    diagnostics.maxShotCandidateScoreByCarrierRole[carrierBucket],
                    candidate.finalScore);
            }
        }
    }

    void recordPhaseTargetRole(
        PhaseDecisionDiagnostics& diagnostics,
        int phaseIndex,
        FormationSlotRole targetRole) {
        switch (phaseDecisionRoleBucket(targetRole)) {
        case PhaseDecisionRoleBucket::CenterBack:
            ++diagnostics.selectedCBTargets[phaseIndex];
            break;
        case PhaseDecisionRoleBucket::Fullback:
            ++diagnostics.selectedFBTargets[phaseIndex];
            break;
        case PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder:
            ++diagnostics.selectedDMCMTargets[phaseIndex];
            break;
        case PhaseDecisionRoleBucket::Winger:
            ++diagnostics.selectedWingerTargets[phaseIndex];
            break;
        case PhaseDecisionRoleBucket::Striker:
            ++diagnostics.selectedSTTargets[phaseIndex];
            break;
        case PhaseDecisionRoleBucket::Other:
            break;
        }
    }

    void recordPhaseDecisionSelection(
        MatchEngineResult& result,
        MatchTeamPhase phase,
        const ActionCandidate& action,
        FormationSlotRole carrierRole,
        FormationSlotRole targetRole,
        PitchPoint ballPosition,
        AttackingDirection attackingDirection) {
        PhaseDecisionDiagnostics& diagnostics = result.phaseDiagnostics.phaseDecisionDiagnostics;
        const int phaseIndex = matchTeamPhaseIndex(phase);
        const double forward = forwardMetersForDirection(
            ballPosition,
            action.intendedTarget,
            attackingDirection);
        const bool recycle =
            action.type == BallCarrierActionType::BackPass
            || (isPassLike(action.type) && forward <= -1.0);
        const bool progressivePass = isPassLike(action.type) && forward >= 6.0;

        if (isPassLike(action.type)) {
            ++diagnostics.selectedPasses[phaseIndex];
            if (isFinalBall(action.type)) {
                ++diagnostics.selectedFinalBalls[phaseIndex];
                ++diagnostics.selectedFinalBallsByCarrierRole[
                    phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(carrierRole))];
                if (action.type == BallCarrierActionType::ThroughBall) {
                    ++diagnostics.selectedThroughBalls;
                } else if (action.type == BallCarrierActionType::Cutback) {
                    ++diagnostics.selectedCutbacks;
                }
            }
            if (recycle) {
                ++diagnostics.selectedRecyclePasses[phaseIndex];
            }
            if (progressivePass) {
                ++diagnostics.selectedProgressivePasses[phaseIndex];
            }
            recordPhaseTargetRole(diagnostics, phaseIndex, targetRole);
        } else if (isCarryLike(action.type)) {
            ++diagnostics.selectedCarryLikeActions[phaseIndex];
            if (action.type == BallCarrierActionType::Carry) {
                ++diagnostics.selectedCarryActions[phaseIndex];
            } else if (action.type == BallCarrierActionType::Dribble) {
                ++diagnostics.selectedDribbleActions[phaseIndex];
            } else if (action.type == BallCarrierActionType::CutInside) {
                ++diagnostics.selectedCutInsideActionsByPhase[phaseIndex];
                ++diagnostics.selectedCutInsideActions;
            }
        } else if (action.type == BallCarrierActionType::Shoot) {
            ++diagnostics.selectedShots[phaseIndex];
            ++diagnostics.selectedShotsByCarrierRole[
                phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(carrierRole))];
        }

        if (phase == MatchTeamPhase::BuildUp) {
            if (isPassLike(action.type)) {
                ++diagnostics.buildUpPassesByRole[
                    phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(carrierRole))];
                if (targetRole != FormationSlotRole::Unknown) {
                    ++diagnostics.buildUpReceptionsByRole[
                        phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(targetRole))];
                }
                if (targetRole == FormationSlotRole::Striker) {
                    ++diagnostics.buildUpSTTargets;
                }
                const PhaseDecisionRoleBucket targetBucket = phaseDecisionRoleBucket(targetRole);
                if (targetBucket == PhaseDecisionRoleBucket::CenterBack
                    || targetBucket == PhaseDecisionRoleBucket::Fullback
                    || targetBucket == PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder) {
                    ++diagnostics.buildUpCBFBDMCMTargets;
                }
            }
            if (action.type == BallCarrierActionType::Shoot) {
                ++diagnostics.buildUpShots;
            }
            if (isFinalBall(action.type)) {
                ++diagnostics.buildUpFinalBalls;
            }
        }

        if (phase == MatchTeamPhase::FinalizingPosition) {
            if (action.type == BallCarrierActionType::Shoot) {
                ++diagnostics.finalizingShotsByRole[
                    phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(carrierRole))];
            }
            if (isFinalBall(action.type)) {
                ++diagnostics.finalizingFinalBallsByRole[
                    phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(carrierRole))];
            }
            if (targetRole == FormationSlotRole::Striker) {
                ++diagnostics.finalizingSTTargets;
            }
            const PhaseDecisionRoleBucket carrierBucket = phaseDecisionRoleBucket(carrierRole);
            const PhaseDecisionRoleBucket targetBucket = phaseDecisionRoleBucket(targetRole);
            if (carrierBucket == PhaseDecisionRoleBucket::Winger
                || carrierBucket == PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder
                || targetBucket == PhaseDecisionRoleBucket::Winger
                || targetBucket == PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder) {
                ++diagnostics.finalizingWingerCMInvolvement;
            }
        }

        if (phase == MatchTeamPhase::CounterAttack) {
            if (isPassLike(action.type) && forward >= 6.0) {
                ++diagnostics.counterSelectedForwardPasses;
            }
            if (isCarryLike(action.type)) {
                ++diagnostics.counterSelectedCarries;
            }
            if (recycle) {
                ++diagnostics.counterSelectedRecyclePasses;
            }
        }
    }

    void recordFinalizingSelectedAction(
        MatchEngineResult& result,
        TeamSimState& team,
        MatchTeamPhase phase,
        const ActionCandidate& action,
        PitchPoint ballPosition,
        AttackingDirection attackingDirection) {
        if (phase != MatchTeamPhase::FinalizingPosition
            || !team.finalizingDiagnosticActive
            || !team.finalizingDiagnosticFromBuildUp) {
            return;
        }

        const int actionIndex = diagnosticActionTypeIndex(action.type);
        if (team.finalizingDiagnosticFirstActionCounted == 0) {
            ++result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics
                .firstFinalizingActionType[actionIndex];
            team.finalizingDiagnosticFirstActionCounted = 1;
        }

        if (team.finalizingDiagnosticFirstThreeActions >= 3) {
            return;
        }

        ++team.finalizingDiagnosticFirstThreeActions;
        if (action.type == BallCarrierActionType::Shoot) {
            ++result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics.firstThreeShots;
        }
        if (isFinalBall(action.type)) {
            ++result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics.firstThreeFinalBalls;
        }
        if (isRecycleAction(action.type, ballPosition, action.intendedTarget, attackingDirection)) {
            ++result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics.firstThreeRecycleActions;
        }
    }

    void recordFinalizingShot(
        MatchEngineResult& result,
        TeamSimState& team,
        MatchTeamPhase phase,
        FormationSlotRole shooterRole,
        double xG) {
        if (phase != MatchTeamPhase::FinalizingPosition) {
            return;
        }
        team.finalizingDiagnosticHadShot = true;
        team.finalizingDiagnosticXG += xG;
        if (shooterRole != FormationSlotRole::Striker) {
            ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingNonSTShots;
            result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingNonSTxG += xG;
        }
    }

    void markFinalizingTurnover(
        MatchEngineResult& result,
        TeamSimState& team,
        MatchTeamPhase phase) {
        if (phase != MatchTeamPhase::FinalizingPosition
            || !team.finalizingDiagnosticActive) {
            return;
        }
        team.finalizingDiagnosticHadTurnover = true;
        if (!team.finalizingDiagnosticFromBuildUp) {
            return;
        }
        if (team.finalizingDiagnosticFirstThreeActions < 3) {
            ++team.finalizingDiagnosticFirstThreeActions;
            ++result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics.firstThreeTurnovers;
        }
    }

    void recordPhaseTurnover(MatchEngineResult& result, MatchTeamPhase phase) {
        ++result.phaseDiagnostics.turnoversByPhase[matchTeamPhaseIndex(phase)];
    }

    void recordPhaseGoal(MatchEngineResult& result, MatchTeamPhase phase) {
        ++result.phaseDiagnostics.goalsByPhase[matchTeamPhaseIndex(phase)];
        if (phase == MatchTeamPhase::CounterAttack) {
            ++result.phaseDiagnostics.counterGoals;
        }
    }

    MatchTeamPhase actingTeamPhaseForDiagnostics(
        const TeamSimState& team,
        const MatchSimulationState& state) {
        if (state.ball.controlState == BallControlState::Controlled
            && state.ball.carrierTeamId == team.teamId
            && !isInPossessionPhase(team.currentPhase)) {
            return MatchTeamPhase::BuildUp;
        }
        return team.currentPhase;
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

    const PlayerGameContext* playerGameContextFor(
        const std::vector<PlayerGameContext>& contexts,
        PlayerId playerId) {
        for (const PlayerGameContext& context : contexts) {
            if (context.playerId == playerId) {
                return &context;
            }
        }
        return nullptr;
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
        state.possession.cleanPossessionRegain = false;
        state.possession.possessionStartedFromLooseBall = false;
        state.possession.possessionRegainSecond = 0;

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
        MatchEngineResult& result,
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
            MatchPlayerSimulationStats& stats = playerStatsFor(result, player.playerId, player.teamId);
            stats.totalDistanceCoveredMeters += movement.distanceMoved;
            stats.offBallMovementDistanceMeters += movement.distanceMoved;
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

    bool isCarryLike(BallCarrierActionType type) {
        return type == BallCarrierActionType::Carry
            || type == BallCarrierActionType::Dribble
            || type == BallCarrierActionType::CutInside;
    }

    bool isFinalBall(BallCarrierActionType type) {
        return type == BallCarrierActionType::ThroughBall
            || type == BallCarrierActionType::LowCross
            || type == BallCarrierActionType::HighCross
            || type == BallCarrierActionType::Cutback;
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

    bool isExplicitFinalBallAction(BallCarrierActionType type) {
        return type == BallCarrierActionType::ThroughBall
            || type == BallCarrierActionType::LowCross
            || type == BallCarrierActionType::Cutback
            || type == BallCarrierActionType::HighCross;
    }

    PitchPoint goalCenterForDirection(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    double forwardMetersForDirection(
        PitchPoint from,
        PitchPoint to,
        AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? to.x - from.x
            : from.x - to.x;
    }

    bool isShootingCorridor(
        PitchPoint point,
        AttackingDirection direction,
        const ChanceCreationProfile& profile) {
        const double goalDistance = PitchGeometry::distance(point, goalCenterForDirection(direction));
        return goalDistance >= profile.finalBallMinimumGoalDistance
            && goalDistance <= profile.finalBallMaximumGoalDistance;
    }

    bool isChanceCreatingFinalBall(
        BallCarrierActionType actionType,
        PitchPoint passStart,
        PitchPoint receivePoint,
        AttackingDirection direction,
        const ChanceCreationProfile& profile) {
        if (isExplicitFinalBallAction(actionType)) {
            return isShootingCorridor(receivePoint, direction, profile);
        }

        if (actionType != BallCarrierActionType::ShortPass
            && actionType != BallCarrierActionType::SwitchPlay) {
            return false;
        }

        return isShootingCorridor(receivePoint, direction, profile)
            && forwardMetersForDirection(passStart, receivePoint, direction)
                >= profile.finalBallMinimumForwardMeters;
    }

    double estimatedPreShotXGAtReceive(
        PitchPoint receivePoint,
        AttackingDirection direction,
        double receiverPressure) {
        return ShotQualityModel::calculateOpenPlayXG(
            receivePoint,
            direction,
            receiverPressure);
    }

    void rememberCompletedPass(
        ChanceCreationTracker& tracker,
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        PlayerId receiverPlayerId,
        PitchPoint passStart,
        PitchPoint passTargetPoint,
        PitchPoint receivePoint,
        AttackingDirection direction,
        double receiverPressure) {
        if (!isPassLike(pending.actionType)
            || pending.sourceTeamId == 0
            || pending.sourcePlayerId == 0
            || receiverPlayerId == 0) {
            clearChanceSource(tracker);
            return;
        }

        const ChanceCreationProfile profile;
        ChanceCreationSource source;
        source.passerPlayerId = pending.sourcePlayerId;
        source.receiverPlayerId = receiverPlayerId;
        source.teamId = pending.sourceTeamId;
        source.actionType = pending.actionType;
        source.passTargetPoint = passTargetPoint;
        source.receivePoint = receivePoint;
        source.second = state.currentSecond;
        source.possessionActionDepth = state.possession.actionDepth;
        source.chanceCreatingFinalBall = isChanceCreatingFinalBall(
            pending.actionType,
            passStart,
            receivePoint,
            direction,
            profile);
        source.estimatedPreShotXGAtReceive = estimatedPreShotXGAtReceive(
            receivePoint,
            direction,
            receiverPressure);
        tracker.active = source;
        tracker.reboundSequenceActive = false;
        tracker.turnoverSequenceActive = false;
    }

    void rememberLooseFinalBallSource(
        ChanceCreationTracker& tracker,
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        PitchPoint passStart,
        PitchPoint passTargetPoint,
        PitchPoint loosePoint,
        AttackingDirection direction,
        double receiverPressure) {
        if (pending.targetPlayerId == 0) {
            clearChanceSource(tracker);
            return;
        }

        const ChanceCreationProfile profile;
        if (!isChanceCreatingFinalBall(
                pending.actionType,
                passStart,
                loosePoint,
                direction,
                profile)) {
            clearChanceSource(tracker);
            return;
        }

        ChanceCreationSource source;
        source.passerPlayerId = pending.sourcePlayerId;
        source.receiverPlayerId = pending.targetPlayerId;
        source.teamId = pending.sourceTeamId;
        source.actionType = pending.actionType;
        source.passTargetPoint = passTargetPoint;
        source.receivePoint = loosePoint;
        source.second = state.currentSecond;
        source.possessionActionDepth = state.possession.actionDepth;
        source.chanceCreatingFinalBall = true;
        source.estimatedPreShotXGAtReceive = estimatedPreShotXGAtReceive(
            loosePoint,
            direction,
            receiverPressure);
        tracker.active = source;
        tracker.reboundSequenceActive = false;
        tracker.turnoverSequenceActive = false;
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
        const bool controlledFromLooseBall =
            state.ball.controlState == BallControlState::Loose;
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
            state.possession.cleanPossessionRegain = !controlledFromLooseBall;
            state.possession.possessionStartedFromLooseBall = controlledFromLooseBall;
            state.possession.possessionRegainSecond = state.currentSecond;
        } else {
            state.possession.isTransition = false;
            state.possession.cleanPossessionRegain = false;
            state.possession.possessionStartedFromLooseBall = controlledFromLooseBall;
        }
    }

    double nearestOpponentPressure(PitchPoint position, const std::vector<PlayerSimState>& opponents);
    double closeGoalAreaPressure(
        PitchPoint position,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction);
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
        state.possession.cleanPossessionRegain = false;
        state.possession.possessionStartedFromLooseBall = false;
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
            nearestOpponentPressure(carrier.position, opponents)
                + blockCompactness * 0.16
                + closeGoalAreaPressure(carrier.position, opponents, shapeContext.attackingDirection),
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

    double centralGoalChannelShare(PitchPoint position) {
        const double distanceFromCenter =
            std::abs(position.y - PitchGeometry::WidthMeters / 2.0);
        return std::clamp(1.0 - (distanceFromCenter / (PitchGeometry::WidthMeters * 0.24)), 0.0, 1.0);
    }

    double opponentGoalDistance(PitchPoint position, AttackingDirection direction) {
        return PitchGeometry::distance(position, opponentGoalCenter(direction));
    }

    double closeGoalAreaPressure(
        PitchPoint position,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction) {
        const double distance = opponentGoalDistance(position, direction);
        if (distance > 14.0) {
            return 0.0;
        }

        const double centralShare = centralGoalChannelShare(position);
        if (centralShare <= 0.05) {
            return 0.0;
        }

        double pressure = 0.0;
        if (distance <= 4.0) {
            pressure += 50.0;
        } else if (distance <= 8.0) {
            pressure += 34.0;
        } else {
            pressure += 16.0;
        }

        for (const PlayerSimState& opponent : opponents) {
            const double opponentDistance = PitchGeometry::distance(position, opponent.position);
            if (opponentDistance <= 6.0) {
                pressure += 7.0;
            } else if (opponentDistance <= 11.0) {
                pressure += 3.0;
            }
        }

        return std::clamp(pressure * centralShare, 0.0, 70.0);
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

    double closeGoalCarryTurnoverProbability(
        PitchPoint target,
        const std::vector<PlayerSimState>& opponents,
        AttackingDirection direction,
        double pressure,
        BallCarrierActionType actionType) {
        const double distance = opponentGoalDistance(target, direction);
        const double centralShare = centralGoalChannelShare(target);
        if (distance > 10.0 || centralShare <= 0.05) {
            return 0.0;
        }

        int nearbyDefenders = 0;
        for (const PlayerSimState& opponent : opponents) {
            if (PitchGeometry::distance(target, opponent.position) <= 7.0) {
                ++nearbyDefenders;
            }
        }

        const double depthRisk = (10.0 - distance) / 10.0;
        const double dribbleRisk =
            actionType == BallCarrierActionType::Dribble
                || actionType == BallCarrierActionType::CutInside ? 0.06 : 0.0;
        return std::clamp(
            0.03
                + depthRisk * 0.24
                + centralShare * 0.10
                + std::clamp(pressure, 0.0, 100.0) / 100.0 * 0.28
                + std::min(nearbyDefenders, 5) * 0.035
                + dribbleRisk,
            0.0,
            0.58);
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

    PitchPoint goalkeeperSaveContactPoint(
        const BallTrajectory& trajectory,
        AttackingDirection attackingDirection,
        const PlayerSimState* goalkeeper,
        const ShotFlowTuning& tuning) {
        const double goalLineX = attackingDirection == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters
            : 0.0;
        const double contactX = goalLineX
            + (attackingDirection == AttackingDirection::HomeToAway
                ? -tuning.saveContactGoalLineOffsetMeters
                : tuning.saveContactGoalLineOffsetMeters);

        double lineY = trajectory.actualTarget.y;
        const double deltaX = trajectory.actualTarget.x - trajectory.start.x;
        if (std::abs(deltaX) > tuning.minimumSaveContactDeltaX) {
            const double progress = clampDouble((contactX - trajectory.start.x) / deltaX, 0.0, 1.0);
            lineY = trajectory.start.y
                + (trajectory.actualTarget.y - trajectory.start.y) * progress;
        }

        PitchPoint contact{ contactX, lineY };
        if (goalkeeper != nullptr) {
            contact.x = contact.x * (1.0 - tuning.saveContactGoalkeeperBlend)
                + goalkeeper->position.x * tuning.saveContactGoalkeeperBlend;
            contact.y = contact.y * (1.0 - tuning.saveContactGoalkeeperBlend)
                + goalkeeper->position.y * tuning.saveContactGoalkeeperBlend;
        }

        const double centerY = PitchGeometry::WidthMeters / 2.0;
        const double allowedHalfWidth =
            (PitchGeometry::GoalWidthMeters / 2.0) + tuning.saveContactLateralPaddingMeters;
        contact.y = clampDouble(contact.y, centerY - allowedHalfWidth, centerY + allowedHalfWidth);
        return PitchGeometry::clampToPitch(contact);
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

    MatchGoalSourceCategory goalSourceCategoryFor(
        const PendingBallAction& pending,
        PlayerId assistPlayerId) {
        if (pending.shotFromRebound) {
            return MatchGoalSourceCategory::Rebound;
        }
        if (pending.shotFromTurnover) {
            return MatchGoalSourceCategory::Turnover;
        }
        if (assistPlayerId != 0 && pending.shotFromReceiverCarry) {
            return MatchGoalSourceCategory::CarryAfterReceive;
        }
        if (assistPlayerId != 0
            && pending.shotFromFinalBall
            && isExplicitFinalBallAction(pending.chanceSourceActionType)) {
            return MatchGoalSourceCategory::AssistedFinalBall;
        }
        if (assistPlayerId != 0) {
            return MatchGoalSourceCategory::AssistedSimplePass;
        }
        return MatchGoalSourceCategory::SoloCarry;
    }

    const char* roleNameForDiagnostic(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            return "GK";
        case FormationSlotRole::CenterBack:
            return "CB";
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            return "FB/WB";
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::AttackingMidfielder:
            return "DM/CM/AM";
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            return "Wide";
        case FormationSlotRole::Striker:
            return "ST";
        case FormationSlotRole::Unknown:
            return "Unknown";
        }
        return "Unknown";
    }

    int diagnosticActionTypeIndex(BallCarrierActionType type) {
        return std::clamp(
            static_cast<int>(type),
            0,
            MatchDiagnosticActionTypeCount - 1);
    }

    int diagnosticRoleBucketIndex(MatchDiagnosticRoleBucket bucket) {
        return static_cast<int>(bucket);
    }

    MatchDiagnosticRoleBucket diagnosticRoleBucket(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::CenterBack:
            return MatchDiagnosticRoleBucket::CenterBack;
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            return MatchDiagnosticRoleBucket::FullbackWingback;
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::AttackingMidfielder:
            return MatchDiagnosticRoleBucket::CentralMidfield;
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            return MatchDiagnosticRoleBucket::Winger;
        case FormationSlotRole::Striker:
            return MatchDiagnosticRoleBucket::Striker;
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::Unknown:
            return MatchDiagnosticRoleBucket::GoalkeeperOrOther;
        }
        return MatchDiagnosticRoleBucket::GoalkeeperOrOther;
    }

    FormationSlotRole assignedRoleForPlayer(
        const MatchEngineInput& input,
        TeamId teamId,
        PlayerId playerId) {
        if (const MatchTeamSnapshot* team = snapshotForTeam(input, teamId)) {
            return assignedRoleFor(team->teamSheet, playerId);
        }
        return FormationSlotRole::Unknown;
    }

    bool isLeftWingerRole(FormationSlotRole role) {
        return role == FormationSlotRole::LeftWinger
            || role == FormationSlotRole::LeftMidfielder;
    }

    bool isRightWingerRole(FormationSlotRole role) {
        return role == FormationSlotRole::RightWinger
            || role == FormationSlotRole::RightMidfielder;
    }

    bool isWingerRole(FormationSlotRole role) {
        return isLeftWingerRole(role) || isRightWingerRole(role);
    }

    bool isFullbackRole(FormationSlotRole role) {
        return role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack;
    }

    bool isCentralMidfieldRole(FormationSlotRole role) {
        return role == FormationSlotRole::DefensiveMidfielder
            || role == FormationSlotRole::CentralMidfielder
            || role == FormationSlotRole::AttackingMidfielder;
    }

    double attackingProgressFor(PitchPoint point, AttackingDirection direction);
    bool isPenaltyAreaForDirection(PitchPoint point, AttackingDirection direction);
    bool isWideChannel(PitchPoint point);
    bool isHalfSpace(PitchPoint point);

    int supportRoleBucketIndex(FormationSlotRole role) {
        return phaseDecisionRoleBucketIndex(phaseDecisionRoleBucket(role));
    }

    PlayerIntentType supportIntentTypeFor(OffBallEventType type) {
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
        case OffBallEventType::BackPassSupport:
            return PlayerIntentType::DropForPass;
        case OffBallEventType::CounterForwardRun:
            return PlayerIntentType::MakeRunBehind;
        case OffBallEventType::DefensiveRecoveryRun:
            return PlayerIntentType::RecoverToGoal;
        case OffBallEventType::RestDefenseHold:
            return PlayerIntentType::HoldAttackingShape;
        case OffBallEventType::None:
            return PlayerIntentType::None;
        }
        return PlayerIntentType::MoveToSupport;
    }

    void recordSupportLifecycleEvent(
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const OffBallSupportEvent& event,
        bool created) {
        const int typeIndex = offBallEventTypeIndex(event.eventType);
        MatchOffBallSupportDiagnostics& diagnostics =
            result.phaseDiagnostics.offBallSupportDiagnostics;
        if (created) {
            ++diagnostics.eventsCreatedByType[typeIndex];
            const FormationSlotRole role = assignedRoleForPlayer(input, event.teamId, event.playerId);
            ++diagnostics.eventsByRole[supportRoleBucketIndex(role)][typeIndex];
            if (isWingerRole(role) && event.eventType == OffBallEventType::CutInsideRun) {
                ++diagnostics.wingerCutInsideEvents;
            }
            if (isWingerRole(role) && event.eventType == OffBallEventType::FarPostRun) {
                ++diagnostics.wingerFarPostEvents;
            }
            if (isFullbackRole(role) && event.eventType == OffBallEventType::OverlapRun) {
                ++diagnostics.fullbackOverlapEvents;
            }
            if (isFullbackRole(role) && event.eventType == OffBallEventType::UnderlapRun) {
                ++diagnostics.fullbackUnderlapEvents;
            }
            if (isFullbackRole(role) && event.eventType == OffBallEventType::RestDefenseHold) {
                ++diagnostics.farSideFullbackRestDefenseHolds;
            }
            return;
        }

        if (event.completionReason == OffBallEventCompletionReason::ReachedRegion) {
            ++diagnostics.eventsCompletedByType[typeIndex];
            ++diagnostics.completedByReachingRegion;
        } else {
            ++diagnostics.eventsExpiredByType[typeIndex];
        }
        diagnostics.activeEventDurationTotal +=
            static_cast<double>(std::max(0, event.completedSecond - event.startSecond));
        ++diagnostics.activeEventDurationSamples;
        switch (event.completionReason) {
        case OffBallEventCompletionReason::PossessionLoss:
            ++diagnostics.expiredOnPossessionLoss;
            break;
        case OffBallEventCompletionReason::Shot:
            ++diagnostics.expiredOnShot;
            break;
        case OffBallEventCompletionReason::OpponentControl:
            ++diagnostics.expiredOnOpponentControl;
            break;
        case OffBallEventCompletionReason::PhaseChange:
            ++diagnostics.expiredOnPhaseChange;
            break;
        case OffBallEventCompletionReason::Timeout:
            ++diagnostics.expiredOnTimeout;
            break;
        case OffBallEventCompletionReason::RestDefenseUnsafe:
            ++diagnostics.cancelledByRestDefense;
            break;
        case OffBallEventCompletionReason::None:
        case OffBallEventCompletionReason::ReachedRegion:
        case OffBallEventCompletionReason::BallOut:
        case OffBallEventCompletionReason::BecameBallCarrier:
        case OffBallEventCompletionReason::Replaced:
            break;
        }
    }

    void appendSupportChainExample(
        MatchEngineResult& result,
        const OffBallSupportEvent& event,
        BallCarrierActionType nextAction,
        bool producedShot,
        double shotXG);

    void recordSupportLifecycleResult(
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const OffBallLifecycleResult& lifecycleResult) {
        for (const OffBallSupportEvent& event : lifecycleResult.completed) {
            recordSupportLifecycleEvent(result, input, event, false);
            appendSupportChainExample(result, event, BallCarrierActionType::Hold, false, 0.0);
        }
        for (const OffBallSupportEvent& event : lifecycleResult.expired) {
            recordSupportLifecycleEvent(result, input, event, false);
            appendSupportChainExample(result, event, BallCarrierActionType::Hold, false, 0.0);
        }
    }

    void appendSupportChainExample(
        MatchEngineResult& result,
        const OffBallSupportEvent& event,
        BallCarrierActionType nextAction,
        bool producedShot,
        double shotXG) {
        constexpr std::size_t MaxExamples = 8;
        if (result.phaseDiagnostics.offBallEventChains.size() >= MaxExamples) {
            return;
        }
        result.phaseDiagnostics.offBallEventChains.push_back(MatchOffBallEventChainDiagnostic{
            eventMinuteForSecond(event.completedSecond > 0 ? event.completedSecond : event.startSecond),
            event.sourcePhase,
            event.teamId,
            event.playerId,
            event.eventType,
            event.targetRegion.preferredLane,
            event.targetRegion.preferredDepth,
            event.resolvedTargetPoint,
            event.completionReason,
            nextAction,
            producedShot,
            shotXG
        });
    }

    void recordSupportGenerationResult(
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const OffBallSupportModelResult& modelResult) {
        MatchOffBallSupportDiagnostics& diagnostics =
            result.phaseDiagnostics.offBallSupportDiagnostics;
        if (modelResult.restDefenseStableBeforeSupport) {
            ++diagnostics.restDefenseStableBeforeSupport;
        }
        if (modelResult.restDefenseStableAfterSupport) {
            ++diagnostics.restDefenseStableAfterSupport;
        } else if (!modelResult.events.empty()) {
            ++diagnostics.restDefenseBreaksAfterSupport;
        }
        diagnostics.supportEventsRejectedByRestDefense += modelResult.rejectedByRestDefense;
        diagnostics.cancelledByRestDefense += modelResult.rejectedByRestDefense;
        for (const OffBallSupportEvent& event : modelResult.events) {
            recordSupportLifecycleEvent(result, input, event, true);
        }
    }

    void applySupportEventsToTargets(
        std::vector<PlayerShapeTarget>& targets,
        const OffBallEventLifecycle& lifecycle) {
        for (PlayerShapeTarget& target : targets) {
            const OffBallSupportEvent* event = lifecycle.activeEventForPlayer(target.playerId);
            if (event == nullptr || event->teamId != target.teamId) {
                continue;
            }
            const double weight = isRestDefenseEvent(event->eventType) ? 0.72 : 0.86;
            target.finalTarget = PitchGeometry::clampToPitch(PitchPoint{
                target.finalTarget.x + (event->resolvedTargetPoint.x - target.finalTarget.x) * weight,
                target.finalTarget.y + (event->resolvedTargetPoint.y - target.finalTarget.y) * weight
            });
        }
    }

    void applySupportEventsToIntents(
        std::vector<ResolvedPlayerIntent>& intents,
        const OffBallEventLifecycle& lifecycle) {
        for (ResolvedPlayerIntent& intent : intents) {
            const OffBallSupportEvent* event = lifecycle.activeEventForPlayer(intent.playerId);
            if (event == nullptr || event->teamId != intent.teamId) {
                continue;
            }
            intent.finalTarget = PitchGeometry::clampToPitch(event->resolvedTargetPoint);
            intent.intent.target = intent.finalTarget;
            intent.intent.type = supportIntentTypeFor(event->eventType);
            intent.intent.relatedPlayerId = 0;
            intent.intent.urgency = isRestDefenseEvent(event->eventType) ? 0.42 : 0.78;
            intent.score += event->priority;
        }
    }

    void recordBothFullbacksAdvanced(
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const OffBallEventLifecycle& lifecycle) {
        for (const MatchTeamSnapshot* team : { &input.homeTeam, &input.awayTeam }) {
            int advancedFullbackEvents = 0;
            for (const OffBallSupportEvent& event : lifecycle.activeEventsForTeam(team->teamId)) {
                if (event.eventType != OffBallEventType::OverlapRun
                    && event.eventType != OffBallEventType::UnderlapRun) {
                    continue;
                }
                if (isFullbackRole(assignedRoleFor(team->teamSheet, event.playerId))) {
                    ++advancedFullbackEvents;
                }
            }
            if (advancedFullbackEvents >= 2) {
                ++result.phaseDiagnostics.offBallSupportDiagnostics.bothFullbacksAdvancedCount;
            }
        }
    }

    bool supportActiveOrRecent(
        const OffBallEventLifecycle* lifecycle,
        PlayerId playerId,
        int currentSecond) {
        return lifecycle != nullptr
            && lifecycle->hadRecentSupportEvent(playerId, currentSecond);
    }

    void recordSupportReceptionImpact(
        MatchEngineResult& result,
        const OffBallEventLifecycle* lifecycle,
        FormationSlotRole role,
        PlayerId playerId,
        PitchPoint receivePoint,
        AttackingDirection direction,
        int currentSecond) {
        if (!supportActiveOrRecent(lifecycle, playerId, currentSecond)) {
            return;
        }
        const double progress = attackingProgressFor(receivePoint, direction);
        const bool finalThird = progress >= 0.66;
        if (isWingerRole(role)) {
            if (isPenaltyAreaForDirection(receivePoint, direction)) {
                ++result.phaseDiagnostics.offBallSupportDiagnostics.wingerBoxReceptionsAfterEvent;
            }
            if (isHalfSpace(receivePoint)) {
                ++result.phaseDiagnostics.offBallSupportDiagnostics.wingerHalfSpaceReceptionsAfterEvent;
            }
        }
        if (isFullbackRole(role) && finalThird && isWideChannel(receivePoint) && progress >= 0.72) {
            ++result.phaseDiagnostics.offBallSupportDiagnostics.fullbackAdvancedWideReceptionsAfterEvent;
        }
    }

    void recordSupportActionImpact(
        MatchEngineResult& result,
        const OffBallEventLifecycle* lifecycle,
        FormationSlotRole carrierRole,
        PlayerId carrierPlayerId,
        BallCarrierActionType actionType,
        int currentSecond) {
        if (!supportActiveOrRecent(lifecycle, carrierPlayerId, currentSecond)) {
            return;
        }
        if (isFinalBall(actionType)) {
            ++result.phaseDiagnostics.offBallSupportDiagnostics.finalBallsAfterSupportEvent;
            if (isFullbackRole(carrierRole)) {
                ++result.phaseDiagnostics.offBallSupportDiagnostics.fullbackFinalBallsAfterEvent;
            }
        }
        if (actionType == BallCarrierActionType::Cutback) {
            ++result.phaseDiagnostics.offBallSupportDiagnostics.cutbacksAfterSupportEvent;
        } else if (actionType == BallCarrierActionType::ThroughBall) {
            ++result.phaseDiagnostics.offBallSupportDiagnostics.throughBallsAfterSupportEvent;
        }
    }

    void recordSupportShotImpact(
        MatchEngineResult& result,
        const OffBallEventLifecycle* lifecycle,
        FormationSlotRole shooterRole,
        PlayerId shooterPlayerId,
        double shotXG,
        int currentSecond) {
        if (!supportActiveOrRecent(lifecycle, shooterPlayerId, currentSecond)) {
            return;
        }
        MatchOffBallSupportDiagnostics& diagnostics =
            result.phaseDiagnostics.offBallSupportDiagnostics;
        ++diagnostics.shotsAfterSupportEvent;
        diagnostics.xGAfterSupportEvent += shotXG;
        if (isWingerRole(shooterRole)) {
            ++diagnostics.wingerShotsAfterSupportEvent;
            ++diagnostics.wingerShotsAfterEvent;
            diagnostics.wingerXGAfterEvent += shotXG;
        }
        if (isCentralMidfieldRole(shooterRole)) {
            ++diagnostics.CMShotsAfterSupportEvent;
        }
    }

    void recordSupportShotAssistImpact(
        MatchEngineResult& result,
        const OffBallEventLifecycle* lifecycle,
        FormationSlotRole providerRole,
        PlayerId providerPlayerId,
        int currentSecond) {
        if (!supportActiveOrRecent(lifecycle, providerPlayerId, currentSecond)) {
            return;
        }
        ++result.phaseDiagnostics.offBallSupportDiagnostics.shotAssistsAfterSupportEvent;
        if (isFullbackRole(providerRole)) {
            ++result.phaseDiagnostics.offBallSupportDiagnostics.fullbackShotAssistsAfterSupportEvent;
        }
    }

    double attackingProgressFor(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x / PitchGeometry::LengthMeters
            : (PitchGeometry::LengthMeters - point.x) / PitchGeometry::LengthMeters;
    }

    bool isPenaltyAreaForDirection(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::isInsideAwayPenaltyArea(point)
            : PitchGeometry::isInsideHomePenaltyArea(point);
    }

    bool isWideChannel(PitchPoint point) {
        return point.y <= 18.0 || point.y >= (PitchGeometry::WidthMeters - 18.0);
    }

    bool isHalfSpace(PitchPoint point) {
        return (point.y > 18.0 && point.y <= 28.0)
            || (point.y >= 40.0 && point.y < (PitchGeometry::WidthMeters - 18.0));
    }

    bool isEdgeZone(PitchPoint point, AttackingDirection direction) {
        const double progress = attackingProgressFor(point, direction);
        return progress >= 0.68
            && progress <= 0.86
            && point.y > 22.0
            && point.y < (PitchGeometry::WidthMeters - 22.0);
    }

    bool isRecycleAction(BallCarrierActionType type, PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return type == BallCarrierActionType::BackPass
            || (isPassLike(type) && forwardMetersForDirection(from, to, direction) <= -1.0);
    }

    void resetFinalizingDiagnosticState(TeamSimState& team) {
        team.finalizingDiagnosticActive = false;
        team.finalizingDiagnosticFromBuildUp = false;
        team.finalizingDiagnosticHadShot = false;
        team.finalizingDiagnosticHadTurnover = false;
        team.finalizingDiagnosticFirstActionCounted = 0;
        team.finalizingDiagnosticFirstThreeActions = 0;
        team.finalizingDiagnosticPassesBeforeEntry = 0;
        team.finalizingDiagnosticXG = 0.0;
    }

    void startFinalizingDiagnostic(
        MatchEngineResult& result,
        TeamSimState& team,
        int possessionActionDepth,
        bool fromBuildUp) {
        resetFinalizingDiagnosticState(team);
        team.finalizingDiagnosticActive = true;
        team.finalizingDiagnosticFromBuildUp = fromBuildUp;
        team.finalizingDiagnosticPassesBeforeEntry = std::max(0, possessionActionDepth);
        ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingEntries;
        if (fromBuildUp) {
            ++result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics.buildUpToFinalizingEntries;
            result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics
                .passesBeforeFirstFinalizingActionTotal += team.finalizingDiagnosticPassesBeforeEntry;
            ++result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics
                .passesBeforeFirstFinalizingActionSamples;
        }
    }

    void finishFinalizingDiagnostic(
        MatchEngineResult& result,
        TeamSimState& team,
        MatchTeamPhase nextPhase) {
        if (!team.finalizingDiagnosticActive) {
            return;
        }
        if (team.finalizingDiagnosticHadShot) {
            ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingPossessionsEndingInShot;
        } else if (nextPhase == MatchTeamPhase::BuildUp) {
            ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingPossessionsRecycledToBuildUp;
        } else if (team.finalizingDiagnosticHadTurnover) {
            ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingPossessionsEndingInTurnover;
        }
        if (team.finalizingDiagnosticFromBuildUp) {
            result.phaseDiagnostics.buildUpToFinalizingQualityDiagnostics.finalizingPositionChainXG +=
                team.finalizingDiagnosticXG;
        }
        resetFinalizingDiagnosticState(team);
    }

    MatchWidePlayerInvolvementDiagnostic* wingerDiagnosticFor(
        MatchEngineResult& result,
        FormationSlotRole role) {
        if (isLeftWingerRole(role)) {
            return &result.phaseDiagnostics.wingerInvolvementDiagnostics.left;
        }
        if (isRightWingerRole(role)) {
            return &result.phaseDiagnostics.wingerInvolvementDiagnostics.right;
        }
        return nullptr;
    }

    void recordWideActionInvolvement(
        MatchEngineResult& result,
        FormationSlotRole role,
        BallCarrierActionType actionType) {
        if (MatchWidePlayerInvolvementDiagnostic* winger =
                wingerDiagnosticFor(result, role)) {
            ++winger->actions;
            if (actionType == BallCarrierActionType::CutInside) {
                ++winger->cutInsideActions;
            }
            if (isFinalBall(actionType)) {
                ++winger->finalBalls;
            }
            if (actionType == BallCarrierActionType::LowCross) {
                ++winger->lowCrosses;
            } else if (actionType == BallCarrierActionType::Cutback) {
                ++winger->cutbacks;
            } else if (actionType == BallCarrierActionType::ThroughBall) {
                ++winger->throughBalls;
            }
        }

        if (isFullbackRole(role)) {
            if (actionType == BallCarrierActionType::LowCross) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.lowCrosses;
            } else if (actionType == BallCarrierActionType::Cutback) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.cutbacks;
            }
            if (isFinalBall(actionType)) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.finalBalls;
            }
        }
    }

    void recordReceptionInvolvement(
        MatchEngineResult& result,
        FormationSlotRole role,
        PlayerId playerId,
        PitchPoint receivePoint,
        AttackingDirection direction,
        MatchTeamPhase actingPhase,
        const OffBallEventLifecycle* supportLifecycle,
        int currentSecond) {
        const double progress = attackingProgressFor(receivePoint, direction);
        const bool finalThird = progress >= 0.66;
        const bool wideFinalThird = finalThird && isWideChannel(receivePoint);
        const bool halfSpace = finalThird && isHalfSpace(receivePoint);
        const bool box = isPenaltyAreaForDirection(receivePoint, direction);

        if (MatchWidePlayerInvolvementDiagnostic* winger =
                wingerDiagnosticFor(result, role)) {
            ++winger->receptions;
            if (finalThird) {
                ++winger->finalThirdReceptions;
            }
            if (wideFinalThird) {
                ++winger->wideFinalThirdReceptions;
            }
            if (halfSpace) {
                ++winger->halfSpaceReceptions;
            }
            if (box) {
                ++winger->boxReceptions;
                ++winger->penaltyAreaReceptions;
            }
        }

        if (isFullbackRole(role)) {
            ++result.phaseDiagnostics.fullbackSupportDiagnostics.receptions;
            if (finalThird) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.finalThirdReceptions;
            }
            if (wideFinalThird) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.wideFinalThirdReceptions;
            }
            if (wideFinalThird && progress >= 0.72) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.advancedWideReceptions;
            }
        }

        if (actingPhase == MatchTeamPhase::FinalizingPosition) {
            if (isWingerRole(role)) {
                ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingWingerReceptions;
            } else if (isFullbackRole(role)) {
                ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingFullbackReceptions;
            } else if (role == FormationSlotRole::Striker) {
                ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingSTReceptions;
            } else if (isCentralMidfieldRole(role) && isEdgeZone(receivePoint, direction)) {
                ++result.phaseDiagnostics.finalizingQualityDiagnostics.finalizingCMEdgeReceptions;
            }
        }

        recordSupportReceptionImpact(
            result,
            supportLifecycle,
            role,
            playerId,
            receivePoint,
            direction,
            currentSecond);
    }

    void recordCarryInvolvement(
        MatchEngineResult& result,
        FormationSlotRole role,
        PitchPoint start,
        PitchPoint end,
        AttackingDirection direction,
        BallCarrierActionType actionType) {
        const double startProgress = attackingProgressFor(start, direction);
        const double endProgress = attackingProgressFor(end, direction);
        const bool endedInBox = isPenaltyAreaForDirection(end, direction);
        const bool enteredFinalThird = startProgress < 0.66 && endProgress >= 0.66;

        if (MatchWidePlayerInvolvementDiagnostic* winger =
                wingerDiagnosticFor(result, role)) {
            if (endedInBox) {
                ++winger->boxCarries;
                ++winger->carriesEndingInBox;
            }
            if (isHalfSpace(end) && endProgress >= 0.62) {
                ++winger->cutInHalfSpaceCarries;
            }
            if (enteredFinalThird) {
                ++winger->successfulFinalThirdEntries;
            }
            if (actionType == BallCarrierActionType::CutInside) {
                ++winger->cutInsideActions;
            }
        }

        if (isFullbackRole(role)) {
            if (enteredFinalThird) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.carriesIntoFinalThird;
            }
            if (endedInBox) {
                ++result.phaseDiagnostics.fullbackSupportDiagnostics.carriesIntoBox;
            }
        }
    }

    void recordShotCreationDiagnostics(
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const PendingBallAction& pending,
        const OffBallEventLifecycle* supportLifecycle,
        int currentSecond) {
        const FormationSlotRole shooterRole =
            assignedRoleForPlayer(input, pending.sourceTeamId, pending.sourcePlayerId);
        const int shooterBucket = diagnosticRoleBucketIndex(diagnosticRoleBucket(shooterRole));
        const int actionIndex = diagnosticActionTypeIndex(pending.chanceSourceActionType);
        ++result.phaseDiagnostics.shotReceiverDiagnostics
            .shotsByShooterRoleAndSourceAction[shooterBucket][actionIndex];
        result.phaseDiagnostics.shotReceiverDiagnostics
            .xGByShooterRoleAndSourceAction[shooterBucket][actionIndex] += pending.shotXG;

        if (MatchWidePlayerInvolvementDiagnostic* winger =
                wingerDiagnosticFor(result, shooterRole)) {
            ++winger->shots;
            winger->xG += pending.shotXG;
        }
        if (isFullbackRole(shooterRole)) {
            ++result.phaseDiagnostics.fullbackSupportDiagnostics.shots;
        }
        recordSupportShotImpact(
            result,
            supportLifecycle,
            shooterRole,
            pending.sourcePlayerId,
            pending.shotXG,
            currentSecond);

        if (pending.assistCandidatePlayerId == 0
            || pending.shotFromRebound
            || pending.shotFromTurnover) {
            return;
        }

        const FormationSlotRole providerRole = assignedRoleForPlayer(
            input,
            pending.sourceTeamId,
            pending.assistCandidatePlayerId);
        const int providerBucket =
            diagnosticRoleBucketIndex(diagnosticRoleBucket(providerRole));
        MatchShotCreationDiagnostics& creation =
            result.phaseDiagnostics.shotCreationDiagnostics;
        ++creation.shotsCreatedByRole[providerBucket];
        creation.xGCreatedByRole[providerBucket] += pending.shotXG;
        ++creation.shotAssistsByRole[providerBucket];
        ++creation.keyPassesByRole[providerBucket];

        if (pending.shotFromFinalBall
            && isExplicitFinalBallAction(pending.chanceSourceActionType)) {
            ++creation.finalBallShotAssistsByRole[providerBucket];
        } else {
            ++creation.simplePassShotAssistsByRole[providerBucket];
        }
        if (pending.chanceSourceActionType == BallCarrierActionType::LowCross) {
            ++creation.lowCrossShotAssistsByRole[providerBucket];
        } else if (pending.chanceSourceActionType == BallCarrierActionType::Cutback) {
            ++creation.cutbackShotAssistsByRole[providerBucket];
        } else if (pending.chanceSourceActionType == BallCarrierActionType::ThroughBall) {
            ++creation.throughBallShotAssistsByRole[providerBucket];
        } else if (pending.chanceSourceActionType == BallCarrierActionType::HighCross) {
            ++creation.highCrossShotAssistsByRole[providerBucket];
        }

        if (MatchWidePlayerInvolvementDiagnostic* winger =
                wingerDiagnosticFor(result, providerRole)) {
            ++winger->shotAssists;
        }
        if (isFullbackRole(providerRole)) {
            ++result.phaseDiagnostics.fullbackSupportDiagnostics.shotAssists;
            result.phaseDiagnostics.fullbackSupportDiagnostics.xGCreated += pending.shotXG;
        }
        recordSupportShotAssistImpact(
            result,
            supportLifecycle,
            providerRole,
            pending.assistCandidatePlayerId,
            currentSecond);
        if (pending.sourcePhase == MatchTeamPhase::FinalizingPosition
            && pending.shotFromFinalBall) {
            ++result.phaseDiagnostics.finalizingQualityDiagnostics
                .finalizingFinalBallShotAssists;
        }
    }

    void recordGoalCreationDiagnostics(
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const PendingBallAction& pending,
        PlayerId assistPlayerId) {
        const FormationSlotRole shooterRole =
            assignedRoleForPlayer(input, pending.sourceTeamId, pending.sourcePlayerId);
        const int shooterBucket = diagnosticRoleBucketIndex(diagnosticRoleBucket(shooterRole));
        const int actionIndex = diagnosticActionTypeIndex(pending.chanceSourceActionType);
        ++result.phaseDiagnostics.shotReceiverDiagnostics
            .goalsByShooterRoleAndSourceAction[shooterBucket][actionIndex];

        if (assistPlayerId == 0) {
            return;
        }

        const FormationSlotRole providerRole =
            assignedRoleForPlayer(input, pending.sourceTeamId, assistPlayerId);
        ++result.phaseDiagnostics.shotCreationDiagnostics.goalsCreatedByRole[
            diagnosticRoleBucketIndex(diagnosticRoleBucket(providerRole))];
    }

    const PlayerSimState* closestOutfieldDefenderToPoint(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam,
        PitchPoint point,
        double& closestDistance) {
        const MatchTeamSnapshot* snapshot = snapshotForTeam(input, defendingTeam.teamId);
        closestDistance = -1.0;
        const PlayerSimState* closestDefender = nullptr;
        if (snapshot == nullptr) {
            return nullptr;
        }
        for (const PlayerSimState& defender : defendingTeam.players) {
            if (assignedRoleFor(snapshot->teamSheet, defender.playerId) == FormationSlotRole::Goalkeeper) {
                continue;
            }
            const double distance = PitchGeometry::distance(defender.position, point);
            if (closestDefender == nullptr || distance < closestDistance) {
                closestDefender = &defender;
                closestDistance = distance;
            }
        }
        return closestDefender;
    }

    double closestOutfieldDefenderDistanceToPoint(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam,
        PitchPoint point) {
        double closest = -1.0;
        closestOutfieldDefenderToPoint(input, defendingTeam, point, closest);
        return closest;
    }

    double closestDefenderDistanceToPoint(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam,
        PitchPoint point) {
        return closestOutfieldDefenderDistanceToPoint(input, defendingTeam, point);
    }

    const char* closestOutfieldDefenderRoleToPoint(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam,
        PitchPoint point) {
        double closest = -1.0;
        const PlayerSimState* defender =
            closestOutfieldDefenderToPoint(input, defendingTeam, point, closest);
        const MatchTeamSnapshot* snapshot = snapshotForTeam(input, defendingTeam.teamId);
        if (defender == nullptr || snapshot == nullptr) {
            return "none";
        }
        return roleNameForDiagnostic(assignedRoleFor(snapshot->teamSheet, defender->playerId));
    }

    double closestAnyDefenderDistanceToPoint(
        const TeamSimState& defendingTeam,
        PitchPoint point) {
        double closest = PitchGeometry::LengthMeters;
        for (const PlayerSimState& defender : defendingTeam.players) {
            closest = std::min(closest, PitchGeometry::distance(defender.position, point));
        }
        return closest;
    }

    std::vector<PressurePlayer> pressurePlayersFor(
        const MatchEngineInput& input,
        const TeamSimState& defendingTeam) {
        std::vector<PressurePlayer> defenders;
        defenders.reserve(defendingTeam.players.size());
        for (const PlayerSimState& defender : defendingTeam.players) {
            if (isAssignedGoalkeeper(input, defendingTeam.teamId, defender.playerId)) {
                continue;
            }
            defenders.push_back(PressurePlayer{
                defender.playerId,
                defender.teamId,
                defender.position,
                defender.currentIntent.type
            });
        }
        return defenders;
    }

    DuelContestant duelContestantFor(
        const MatchEngineInput& input,
        const PlayerSimState* player) {
        DuelContestant contestant;
        if (player == nullptr) {
            return contestant;
        }

        const MatchPlayerSnapshot* snapshot = findSnapshotForPlayer(input, player->playerId);
        contestant.playerId = player->playerId;
        contestant.teamId = player->teamId;
        contestant.position = player->position;
        contestant.intentType = player->currentIntent.type;
        contestant.attributes = snapshot != nullptr ? snapshot->attributes : PlayerAttributes{};
        contestant.baseOverall = snapshot != nullptr ? snapshot->baseOverall : player->baseOverall;
        contestant.fatigue = player->fatigue;
        return contestant;
    }

    const char* assistNoneReasonFor(
        const PendingBallAction& pending,
        PlayerId assistPlayerId) {
        if (assistPlayerId != 0) {
            return "AssistRetained";
        }
        if (pending.shotFromRebound) {
            return "Rebound";
        }
        if (pending.shotFromTurnover) {
            return "Turnover";
        }
        if (pending.shotFromReceiverCarry) {
            return "ExcessiveCarryAfterReceive";
        }
        if (pending.chanceSourceActionType == BallCarrierActionType::Hold) {
            return "NoPriorPass";
        }
        return "SoloCarry";
    }

    void appendGoalChainDiagnostic(
        MatchEngineResult& result,
        const MatchEngineInput& input,
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        const BallTrajectory& trajectory,
        PlayerId assistPlayerId,
        AttackingDirection direction,
        const TeamSimState& defendingTeam) {
        MatchGoalChainDiagnostic chain;
        chain.minute = eventMinuteForSecond(state.currentSecond);
        chain.teamId = pending.sourceTeamId;
        chain.scorerPlayerId = pending.sourcePlayerId;
        chain.assistPlayerId = assistPlayerId;
        chain.sourceActionType = pending.chanceSourceActionType;
        chain.sourceCategory = goalSourceCategoryFor(pending, assistPlayerId);
        chain.shotDistanceMeters =
            PitchGeometry::distance(trajectory.start, goalCenterForDirection(direction));
        chain.preShotXG = pending.shotQuality.preShotXG;
        chain.keeperFacingXG = pending.shotQuality.keeperFacingXG;
        chain.effectiveXG = pending.shotQuality.effectiveXG;
        chain.shotPressure = pending.pressure;
        chain.closestDefenderDistance =
            closestDefenderDistanceToPoint(input, defendingTeam, trajectory.start);
        chain.closestOutfieldDefenderDistance = chain.closestDefenderDistance;
        chain.closestDefenderRole =
            closestOutfieldDefenderRoleToPoint(input, defendingTeam, trajectory.start);
        chain.carryAfterReceiveMeters = pending.carryAfterReceiveMeters;
        chain.touchesAfterReceive = pending.touchesAfterReceive;
        chain.defendersBeatenAfterReceive = pending.defendersBeatenAfterReceive;
        chain.assistRetained = assistPlayerId != 0;
        chain.followedControlledCarryAfterReceive = pending.shotFromReceiverCarry;
        chain.finalBallThroughBall = pending.chanceSourceActionType == BallCarrierActionType::ThroughBall;
        chain.finalBallLowCross = pending.chanceSourceActionType == BallCarrierActionType::LowCross;
        chain.finalBallCutback = pending.chanceSourceActionType == BallCarrierActionType::Cutback;
        chain.finalBallHighCross = pending.chanceSourceActionType == BallCarrierActionType::HighCross;
        chain.finalBallSimplePass =
            pending.chanceSourceActionType == BallCarrierActionType::ShortPass
            || pending.chanceSourceActionType == BallCarrierActionType::SwitchPlay;
        chain.rebound = pending.shotFromRebound;
        chain.transition = pending.shotFromTurnover;
        chain.turnover = pending.shotFromTurnover;
        chain.assistNoneReason = assistNoneReasonFor(pending, assistPlayerId);
        result.goalChains.push_back(chain);
    }

    bool chanceSourceStillValidForShot(
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        const ChanceCreationSource& source) {
        const ChanceCreationProfile profile;
        if (source.teamId != pending.sourceTeamId
            || source.passerPlayerId == 0
            || source.receiverPlayerId != pending.sourcePlayerId
            || source.passerPlayerId == pending.sourcePlayerId) {
            return false;
        }
        if (state.currentSecond - source.second > profile.maximumSecondsAfterReceive) {
            return false;
        }
        if (state.possession.actionDepth - source.possessionActionDepth
            > profile.maximumActionsAfterReceive) {
            return false;
        }
        const int carryLimit = profile.maximumControlledCarries
            + (source.chanceCreatingFinalBall ? 1 : 0);
        const double carryDistanceLimit = profile.maximumControlledCarryDistance
            + (source.chanceCreatingFinalBall ? 6.0 : 0.0);
        if (source.controlledCarryCount > carryLimit
            || source.controlledCarryDistance > carryDistanceLimit) {
            return false;
        }

        return true;
    }

    PlayerId assistCandidateForShot(
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        const ChanceCreationTracker& tracker) {
        if (!tracker.active
            || tracker.reboundSequenceActive
            || !chanceSourceStillValidForShot(state, pending, *tracker.active)) {
            return 0;
        }

        return tracker.active->passerPlayerId;
    }

    bool shotFollowsReceiverCarry(
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        const ChanceCreationTracker& tracker) {
        return tracker.active
            && chanceSourceStillValidForShot(state, pending, *tracker.active)
            && tracker.active->followedByControlledCarry;
    }

    BallCarrierActionType chanceSourceActionTypeForShot(
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        const ChanceCreationTracker& tracker) {
        return tracker.active && chanceSourceStillValidForShot(state, pending, *tracker.active)
            ? tracker.active->actionType
            : BallCarrierActionType::Hold;
    }

    bool shotFromFinalBall(
        const MatchSimulationState& state,
        const PendingBallAction& pending,
        const ChanceCreationTracker& tracker) {
        return tracker.active
            && chanceSourceStillValidForShot(state, pending, *tracker.active)
            && tracker.active->chanceCreatingFinalBall;
    }

    void noteControlledCarryAfterReceive(
        ChanceCreationTracker& tracker,
        const PlayerSimState& carrier,
        PitchPoint start,
        PitchPoint end,
        BallCarrierActionType actionType) {
        if (!tracker.active
            || tracker.active->teamId != carrier.teamId
            || tracker.active->receiverPlayerId != carrier.playerId) {
            return;
        }

        tracker.active->followedByControlledCarry = true;
        ++tracker.active->controlledCarryCount;
        tracker.active->controlledCarryDistance += PitchGeometry::distance(start, end);
        if (actionType == BallCarrierActionType::Dribble
            || actionType == BallCarrierActionType::CutInside) {
            ++tracker.active->defendersBeatenAfterReceive;
        }
    }

    void markReboundChain(ChanceCreationTracker& tracker) {
        tracker.active = std::nullopt;
        tracker.reboundSequenceActive = true;
        tracker.turnoverSequenceActive = false;
    }

    void markTurnoverChain(ChanceCreationTracker& tracker) {
        tracker.active = std::nullopt;
        tracker.reboundSequenceActive = false;
        tracker.turnoverSequenceActive = true;
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
        MatchEngineResult& result,
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
        const PitchPoint previousPosition = receiver->position;
        receiver->position = PitchGeometry::clampToPitch(PitchPoint{
            receiver->position.x + ((trajectory.actualTarget.x - receiver->position.x) * ratio),
            receiver->position.y + ((trajectory.actualTarget.y - receiver->position.y) * ratio)
        });
        const double distanceMoved = PitchGeometry::distance(previousPosition, receiver->position);
        MatchPlayerSimulationStats& receiverStats =
            playerStatsFor(result, receiver->playerId, receiver->teamId);
        receiverStats.totalDistanceCoveredMeters += distanceMoved;
        receiverStats.offBallMovementDistanceMeters += distanceMoved;
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
        const TeamSimState& defendingTeam,
        ChanceCreationTracker& chanceTracker) {
        const PlayerId assistPlayerId =
            pending.shotFromRebound || pending.shotFromTurnover
                ? 0
                : pending.assistCandidatePlayerId;
        ++teamStatsFor(result, pending.sourceTeamId).goals;
        ++playerStatsFor(result, pending.sourcePlayerId, pending.sourceTeamId).goals;
        MatchTeamSimulationStats& scoringStats = teamStatsFor(result, pending.sourceTeamId);
        if (assistPlayerId != 0) {
            ++playerStatsFor(result, assistPlayerId, pending.sourceTeamId).assists;
            ++scoringStats.assistedGoals;
        } else if (pending.shotFromRebound) {
            ++scoringStats.reboundGoals;
        } else if (pending.shotFromTurnover) {
            ++scoringStats.transitionGoals;
        } else {
            ++scoringStats.unassistedOpenPlayGoals;
        }
        recordPhaseGoal(result, pending.sourcePhase);
        recordGoalCreationDiagnostics(result, input, pending, assistPlayerId);
        appendOfficialGoalEvent(result, state, pending, assistPlayerId);
        const AttackingDirection direction =
            state.homeTeam.teamId == pending.sourceTeamId
                ? AttackingDirection::HomeToAway
                : AttackingDirection::AwayToHome;
        appendGoalChainDiagnostic(
            result,
            input,
            state,
            pending,
            trajectory,
            assistPlayerId,
            direction,
            defendingTeam);
        clearChanceChain(chanceTracker);

        if (TeamSimState* scoringTeam = findTeamState(state, pending.sourceTeamId)) {
            ++scoringTeam->goals;
        }

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
        MatchTeamPhase carrierPhase,
        const TeamGameContext& carrierTeamGameContext,
        const std::vector<PlayerGameContext>& carrierPlayerGameContexts,
        const BallTrajectoryBuilder& trajectoryBuilder,
        const BallCarrierDecisionModel& decisionModel,
        const ActionSelector& selector,
        std::optional<PendingBallAction>& pending,
        ChanceCreationTracker& chanceTracker,
        OffBallEventLifecycle& supportLifecycle,
        const RestDefenseModel& restDefenseModel,
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
        TeamSimState* carrierTeamState = findTeamState(state, carrier->teamId);
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
        const PlayerGameContext* carrierGameContext = playerGameContextFor(
            carrierPlayerGameContexts,
            carrier->playerId);
        const std::vector<ActionCandidate> candidates = decisionModel.evaluate(
            BallCarrierDecisionModelContext{
                carrierSnapshot,
                snapshotForTeam(input, opponentState->teamId),
                carrierTeamState,
                opponentState,
                carrier,
                carrierShapeContext.attackingDirection,
                playerDecisionContext,
                carrierPhase,
                &carrierTeamGameContext,
                carrierGameContext
            });
        recordPhaseDecisionCandidates(result, carrierPhase, carrierRole, candidates);
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
        const FormationSlotRole targetRole =
            action.targetPlayerId != 0
                ? assignedRoleFor(carrierSnapshot->teamSheet, action.targetPlayerId)
                : FormationSlotRole::Unknown;
        recordPhaseDecisionSelection(
            result,
            carrierPhase,
            action,
            carrierRole,
            targetRole,
            state.ball.position,
            carrierShapeContext.attackingDirection);
        recordFinalizingSelectedAction(
            result,
            *carrierTeamState,
            carrierPhase,
            action,
            state.ball.position,
            carrierShapeContext.attackingDirection);
            recordWideActionInvolvement(result, carrierRole, actionType);
            recordSupportActionImpact(
                result,
                &supportLifecycle,
                carrierRole,
                carrier->playerId,
                actionType,
                state.currentSecond);
            ++state.possession.actionDepth;
        double executionPressure = isPassLike(actionType)
            ? std::max(
                action.pressurePenalty,
                contextualPassPressure(
                    state.ball.position,
                    action.intendedTarget,
                    opponentState->players))
            : action.pressurePenalty;
        const PressureContext pressureContext = PressureModel{}.build(PressureContextRequest{
            pressurePlayersFor(input, *opponentState),
            state.ball.position,
            action.intendedTarget,
            opponentGoalCenter(carrierShapeContext.attackingDirection),
            carrierShapeContext.attackingDirection,
            actionType
        });
        const PlayerSimState* nearestPressureDefender =
            findPlayerState(state, pressureContext.closestOutfieldDefenderId);
        executionPressure = std::max(
            executionPressure,
            clampDouble(
                pressureContext.pressureStrength * (isPassLike(actionType) ? 0.55 : 0.42),
                0.0,
                100.0));
        if (executionPressure >= 10.0 && nearestPressureDefender != nullptr) {
            ++teamStatsFor(result, nearestPressureDefender->teamId).pressures;
            ++playerStatsFor(
                result,
                nearestPressureDefender->playerId,
                nearestPressureDefender->teamId).pressures;
        }
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
            const PitchPoint carryStart = carrier->position;
            DuelResolutionResult duel = DuelResolver{}.resolve(DuelResolutionRequest{
                actionType,
                duelContestantFor(input, carrier),
                duelContestantFor(input, nearestPressureDefender),
                pressureContext,
                carryStart,
                action.intendedTarget,
                carrierShapeContext.attackingDirection,
                stepSeed(
                    baseSeed,
                        state,
                        static_cast<std::uint64_t>(carrier->playerId)
                            ^ (static_cast<std::uint64_t>(actionType) << 32)
                            ^ 0xd71c0bdULL)
            });
            executionPressure = std::max(
                executionPressure,
                action.pressurePenalty + duel.addedExecutionPressure);

            const bool explicitDribbleAttempt =
                duel.duelOccurred
                && (actionType == BallCarrierActionType::Dribble
                    || actionType == BallCarrierActionType::CutInside);

            if (duel.duelOccurred) {
                if (explicitDribbleAttempt) {
                    ++teamStatsFor(result, carrier->teamId).dribblesAttempted;
                    ++playerStatsFor(result, carrier->playerId, carrier->teamId).dribblesAttempted;
                } else {
                    ++teamStatsFor(result, carrier->teamId).carryUnderPressure;
                    ++playerStatsFor(result, carrier->playerId, carrier->teamId).carryUnderPressure;
                }
                if (nearestPressureDefender != nullptr) {
                    MatchTeamSimulationStats& defendingStats =
                        teamStatsFor(result, nearestPressureDefender->teamId);
                    MatchPlayerSimulationStats& defenderStats = playerStatsFor(
                        result,
                        nearestPressureDefender->playerId,
                        nearestPressureDefender->teamId);
                    ++defendingStats.duels;
                    ++defenderStats.duels;
                    if (duel.tackleAttempted) {
                        ++defendingStats.tacklesAttempted;
                        ++defenderStats.tackleAttempts;
                    }
                }
            }

            const double elapsedSeconds = movementDurationSeconds(
                carrier->position,
                duel.duelOccurred ? duel.resolvedTarget : action.intendedTarget,
                carrier->effectivePace,
                2.0,
                actionType == BallCarrierActionType::Carry ? 6.0 : 5.0);

            if (duel.outcome == DuelOutcomeType::DefenderWinsTackle
                || duel.outcome == DuelOutcomeType::BallLoose) {
                if (nearestPressureDefender != nullptr) {
                    MatchTeamSimulationStats& defendingStats =
                        teamStatsFor(result, nearestPressureDefender->teamId);
                    MatchPlayerSimulationStats& defenderStats = playerStatsFor(
                        result,
                        nearestPressureDefender->playerId,
                        nearestPressureDefender->teamId);
                    ++defendingStats.dispossessionsForced;
                    ++defenderStats.dispossessionsForced;
                    if (duel.outcome == DuelOutcomeType::DefenderWinsTackle) {
                        ++defendingStats.defenderWinsTackle;
                        ++defenderStats.defenderWinsTackle;
                    } else {
                        ++defendingStats.ballLooseFromDuel;
                        ++defenderStats.ballLooseFromDuel;
                    }
                    if (duel.tackleAttempted) {
                        ++defendingStats.tacklesWon;
                        ++defenderStats.tacklesWon;
                        ++defenderStats.tackles;
                    }
                    appendTrace(
                        result,
                        input.options.detail,
                        state,
                        MatchTraceKind::Tackle,
                        nearestPressureDefender->teamId,
                        nearestPressureDefender->playerId,
                        carrier->playerId,
                        carrier->position,
                        duel.loosePoint);
                }
                if (explicitDribbleAttempt) {
                    ++teamStatsFor(result, carrier->teamId).dribblesLost;
                    ++playerStatsFor(result, carrier->playerId, carrier->teamId).dribblesLost;
                }
                if (duel.outcome == DuelOutcomeType::DefenderWinsTackle
                    && nearestPressureDefender != nullptr) {
                    setControlledBy(
                        state,
                        nearestPressureDefender->playerId,
                        nearestPressureDefender->teamId,
                        duel.loosePoint);
                    markTurnoverChain(chanceTracker);
                } else {
                    setLooseBall(state, duel.loosePoint);
                    markTurnoverChain(chanceTracker);
                }
                recordPhaseTurnover(result, carrierPhase);
                markFinalizingTurnover(result, *carrierTeamState, carrierPhase);
                appendTrace(
                    result,
                    input.options.detail,
                    state,
                    MatchTraceKind::Turnover,
                    carrier->teamId,
                    carrier->playerId,
                    0,
                    carrier->position,
                    duel.loosePoint);
                return SimulationStepResult{ elapsedSeconds, true };
            }

            if (duel.duelOccurred && duel.tackleAttempted && nearestPressureDefender != nullptr) {
                MatchTeamSimulationStats& defendingStats =
                    teamStatsFor(result, nearestPressureDefender->teamId);
                MatchPlayerSimulationStats& defenderStats = playerStatsFor(
                    result,
                    nearestPressureDefender->playerId,
                    nearestPressureDefender->teamId);
                ++defendingStats.tacklesLost;
                ++defenderStats.tacklesLost;
            }

            if (duel.outcome == DuelOutcomeType::ForcedSideways
                || duel.outcome == DuelOutcomeType::ForcedBackward) {
                if (nearestPressureDefender != nullptr) {
                    MatchTeamSimulationStats& defendingStats =
                        teamStatsFor(result, nearestPressureDefender->teamId);
                    MatchPlayerSimulationStats& defenderStats = playerStatsFor(
                        result,
                        nearestPressureDefender->playerId,
                        nearestPressureDefender->teamId);
                    if (duel.outcome == DuelOutcomeType::ForcedSideways) {
                        ++defendingStats.forcedSideways;
                        ++defenderStats.forcedSideways;
                    } else {
                        ++defendingStats.forcedBackward;
                        ++defenderStats.forcedBackward;
                    }
                }
            } else if (duel.outcome == DuelOutcomeType::AttackerBeatsDefender) {
                ++teamStatsFor(result, carrier->teamId).attackerBeatsDefender;
                ++playerStatsFor(result, carrier->playerId, carrier->teamId).attackerBeatsDefender;
                if (explicitDribbleAttempt) {
                    ++teamStatsFor(result, carrier->teamId).dribblesWon;
                    ++playerStatsFor(result, carrier->playerId, carrier->teamId).dribblesWon;
                }
            } else if (duel.outcome == DuelOutcomeType::AttackerKeepsBall) {
                ++teamStatsFor(result, carrier->teamId).attackerKeepsUnderPressure;
                ++playerStatsFor(result, carrier->playerId, carrier->teamId).attackerKeepsUnderPressure;
                if (explicitDribbleAttempt) {
                    ++teamStatsFor(result, carrier->teamId).dribblesWon;
                    ++playerStatsFor(result, carrier->playerId, carrier->teamId).dribblesWon;
                }
            }

            moveControlledBall(
                state,
                *carrier,
                duel.duelOccurred ? duel.resolvedTarget : action.intendedTarget,
                (actionType == BallCarrierActionType::Carry ? 5.5 : 4.0) * elapsedSeconds);
            const double carryDistance = PitchGeometry::distance(carryStart, state.ball.position);
            MatchPlayerSimulationStats& carrierStats =
                playerStatsFor(result, carrier->playerId, carrier->teamId);
            carrierStats.totalDistanceCoveredMeters += carryDistance;
            carrierStats.onBallCarryDistanceMeters += carryDistance;
            if (actionType == BallCarrierActionType::Carry) {
                ++carrierStats.carryTraces;
                if (forwardMetersForDirection(
                        carryStart,
                        state.ball.position,
                        carrierShapeContext.attackingDirection) >= 5.0) {
                    ++carrierStats.progressiveCarries;
                }
            } else {
                ++carrierStats.dribbleTraces;
            }
            noteControlledCarryAfterReceive(
                chanceTracker,
                *carrier,
                carryStart,
                state.ball.position,
                actionType);
            recordCarryInvolvement(
                result,
                carrierRole,
                carryStart,
                state.ball.position,
                carrierShapeContext.attackingDirection,
                actionType);
            recordPhaseAction(result, carrierPhase, actionType);
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
        PlayerId assistCandidatePlayerId = 0;
        bool shotFromFinalBallSource = false;
        bool shotFromReceiverCarry = false;
        bool shotFromRebound = false;
        bool shotFromTurnover = false;
        BallCarrierActionType chanceSourceActionType = BallCarrierActionType::Hold;
        double carryAfterReceiveMeters = 0.0;
        int touchesAfterReceive = 0;
        int defendersBeatenAfterReceive = 0;

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
            MatchPlayerSimulationStats& passerStats =
                playerStatsFor(result, carrier->playerId, carrier->teamId);
            ++passerStats.passesAttempted;
            if (actionType == BallCarrierActionType::ShortPass) {
                ++passerStats.shortPassesAttempted;
            } else if (actionType == BallCarrierActionType::ThroughBall) {
                ++passerStats.throughBalls;
                ++passerStats.finalBalls;
            } else if (actionType == BallCarrierActionType::LowCross) {
                ++passerStats.lowCrosses;
                ++passerStats.finalBalls;
            } else if (actionType == BallCarrierActionType::Cutback) {
                ++passerStats.cutbacks;
                ++passerStats.finalBalls;
            } else if (actionType == BallCarrierActionType::HighCross) {
                ++passerStats.finalBalls;
            }
        } else if (actionType == BallCarrierActionType::Clear) {
            ++teamStatsFor(result, carrier->teamId).clearances;
            ++playerStatsFor(result, carrier->playerId, carrier->teamId).clearances;
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
        } else if (actionType == BallCarrierActionType::Shoot) {
            PendingBallAction shotPendingForSource = pendingPlayerAction(
                carrier->teamId,
                carrier->playerId,
                action.targetPlayerId,
                actionType,
                trajectoryType,
                executionQuality,
                executionPressure,
                true,
                0.0,
                ShotType::ControlledFinish,
                ShotContext{},
                ShotTargetSelectionResult{},
                ShotExecutionResult{},
                ShotQualityResult{},
                0,
                false,
                false,
                false,
                false,
                BallCarrierActionType::Hold,
                0.0,
                0,
                0,
                carrierPhase);
            assistCandidatePlayerId =
                assistCandidateForShot(state, shotPendingForSource, chanceTracker);
            shotFromFinalBallSource =
                shotFromFinalBall(state, shotPendingForSource, chanceTracker);
            shotFromReceiverCarry =
                shotFollowsReceiverCarry(state, shotPendingForSource, chanceTracker);
            shotFromRebound = chanceTracker.reboundSequenceActive;
            shotFromTurnover =
                !shotFromRebound
                && (chanceTracker.turnoverSequenceActive || state.possession.isTransition)
                && state.possession.actionDepth <= 4;
            chanceSourceActionType =
                chanceSourceActionTypeForShot(state, shotPendingForSource, chanceTracker);
            if (chanceTracker.active
                && chanceSourceStillValidForShot(state, shotPendingForSource, *chanceTracker.active)) {
                carryAfterReceiveMeters = chanceTracker.active->controlledCarryDistance;
                touchesAfterReceive = chanceTracker.active->controlledCarryCount;
                defendersBeatenAfterReceive = chanceTracker.active->defendersBeatenAfterReceive;
            }
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
            if (goalkeeper != nullptr) {
                shotContext.goalkeeperPosition = goalkeeper->position;
            }
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
            shotXG = shotQuality.effectiveXG;
            ++teamStatsFor(result, carrier->teamId).shots;
            MatchTeamSimulationStats& shootingStats = teamStatsFor(result, carrier->teamId);
            shootingStats.expectedGoals += shotXG;
            shootingStats.rawExpectedGoals += shotQuality.rawXG;
            shootingStats.preShotExpectedGoals += shotQuality.preShotXG;
            shootingStats.keeperFacingExpectedGoals += shotQuality.keeperFacingXG;
            shootingStats.blockedExpectedGoals +=
                std::max(0.0, shotQuality.keeperFacingXG - shotQuality.effectiveXG);
            if (assistCandidatePlayerId != 0) {
                ++shootingStats.assistedShots;
                ++playerStatsFor(result, carrier->playerId, carrier->teamId).assistedShotsReceived;
            } else if (shotFromRebound) {
                ++shootingStats.reboundShots;
            } else if (shotFromTurnover) {
                ++shootingStats.turnoverShots;
            } else {
                ++shootingStats.soloCarryShots;
            }
            if (shotFromFinalBallSource) {
                ++shootingStats.finalBallShots;
                ++playerStatsFor(result, carrier->playerId, carrier->teamId).finalBallShotsReceived;
            }
            if (chanceSourceActionType == BallCarrierActionType::Cutback) {
                ++shootingStats.cutbackShots;
            } else if (chanceSourceActionType == BallCarrierActionType::ThroughBall) {
                ++shootingStats.throughBallShots;
            } else if (chanceSourceActionType == BallCarrierActionType::LowCross) {
                ++shootingStats.lowCrossShots;
            }
            MatchPlayerSimulationStats& shooterStats =
                playerStatsFor(result, carrier->playerId, carrier->teamId);
            ++shooterStats.shots;
            shooterStats.expectedGoals += shotQuality.effectiveXG;
            shooterStats.preShotExpectedGoals += shotQuality.preShotXG;
            PendingBallAction shotDiagnosticPending = pendingPlayerAction(
                carrier->teamId,
                carrier->playerId,
                action.targetPlayerId,
                actionType,
                trajectoryType,
                executionQuality,
                executionPressure,
                true,
                shotXG,
                shotType,
                shotContext,
                shotTarget,
                shotExecution,
                shotQuality,
                assistCandidatePlayerId,
                shotFromFinalBallSource,
                shotFromReceiverCarry,
                shotFromRebound,
                shotFromTurnover,
                chanceSourceActionType,
                carryAfterReceiveMeters,
                touchesAfterReceive,
                defendersBeatenAfterReceive,
                carrierPhase);
            recordShotCreationDiagnostics(
                result,
                input,
                shotDiagnosticPending,
                &supportLifecycle,
                state.currentSecond);
            recordFinalizingShot(result, *carrierTeamState, carrierPhase, carrierRole, shotXG);
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

        recordPhaseAction(result, carrierPhase, actionType, shotXG);

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
            shotQuality,
            actionType == BallCarrierActionType::Shoot ? assistCandidatePlayerId : 0,
            actionType == BallCarrierActionType::Shoot && shotFromFinalBallSource,
            actionType == BallCarrierActionType::Shoot && shotFromReceiverCarry,
            actionType == BallCarrierActionType::Shoot && shotFromRebound,
            actionType == BallCarrierActionType::Shoot && shotFromTurnover,
            actionType == BallCarrierActionType::Shoot
                ? chanceSourceActionType
                : BallCarrierActionType::Hold,
            actionType == BallCarrierActionType::Shoot ? carryAfterReceiveMeters : 0.0,
            actionType == BallCarrierActionType::Shoot ? touchesAfterReceive : 0,
            actionType == BallCarrierActionType::Shoot ? defendersBeatenAfterReceive : 0,
            carrierPhase);

        if (actionType == BallCarrierActionType::Shoot) {
            if (const OffBallSupportEvent* supportEvent =
                    supportLifecycle.activeEventForPlayer(carrier->playerId)) {
                appendSupportChainExample(
                    result,
                    *supportEvent,
                    BallCarrierActionType::Shoot,
                    true,
                    shotXG);
            }
            recordSupportLifecycleResult(
                result,
                input,
                supportLifecycle.expireForShot(carrier->teamId, state.currentSecond));
        }

        appendTrace(
            result,
            input.options.detail,
            state,
            traceKindFor(actionType),
            carrier->teamId,
            carrier->playerId,
            actionType == BallCarrierActionType::Shoot
                ? assistCandidatePlayerId
                : action.targetPlayerId,
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
        ChanceCreationTracker& chanceTracker) {
        setControlledBy(state, defender.playerId, defender.teamId, winPoint);
        if (isPassLike(pending.actionType)) {
            if (isAssignedGoalkeeper(input, defender.teamId, defender.playerId)) {
                ++teamStatsFor(result, defender.teamId).keeperClaims;
                ++playerStatsFor(result, defender.playerId, defender.teamId).keeperClaims;
                ++teamStatsFor(result, pending.sourceTeamId).passesKeeperClaimed;
            } else {
                ++teamStatsFor(result, defender.teamId).interceptions;
                ++teamStatsFor(result, defender.teamId).passInterceptions;
                ++playerStatsFor(result, defender.playerId, defender.teamId).interceptions;
                ++playerStatsFor(result, defender.playerId, defender.teamId).passInterceptions;
                ++teamStatsFor(result, pending.sourceTeamId).passesIntercepted;
            }
        }
        recordPhaseTurnover(result, pending.sourcePhase);
        markTurnoverChain(chanceTracker);
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
        PassResolutionOutcome outcome,
        PitchPoint loosePoint,
        PitchPoint passStart,
        PitchPoint passTargetPoint,
        ChanceCreationTracker& chanceTracker) {
        if (isPassLike(pending.actionType)) {
            if (outcome == PassResolutionOutcome::OutOfPlay) {
                ++teamStatsFor(result, pending.sourceTeamId).passesOutOfPlay;
            } else if (outcome == PassResolutionOutcome::DeflectedLoose) {
                ++teamStatsFor(result, pending.sourceTeamId).passesDeflected;
            } else {
                ++teamStatsFor(result, pending.sourceTeamId).passesLoose;
            }
        }
        recordPhaseTurnover(result, pending.sourcePhase);
        setLooseBall(state, loosePoint);
        const TeamSimState* sourceTeam = findTeamState(state, pending.sourceTeamId);
        if (sourceTeam != nullptr) {
            rememberLooseFinalBallSource(
                chanceTracker,
                state,
                pending,
                passStart,
                passTargetPoint,
                loosePoint,
                attackingDirectionForTeam(*sourceTeam),
                pending.pressure);
        } else {
            clearChanceSource(chanceTracker);
        }
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
        ChanceCreationTracker& chanceTracker,
        const OffBallEventLifecycle& supportLifecycle) {
        const double elapsedToShot = remainingTrajectorySeconds(state, trajectory);
        if (!pending || !pending->isShot) {
            return SimulationStepResult{ elapsedToShot };
        }

        const TeamSimState* attackingTeam = findTeamState(state, pending->sourceTeamId);
        if (attackingTeam == nullptr) {
            setLooseBall(state, trajectory.actualTarget);
            clearChanceChain(chanceTracker);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToShot };
        }

        const AttackingDirection direction = attackingDirectionForTeam(*attackingTeam);
        const PlayerSimState* goalkeeper = findGoalkeeperOrNearestOwnGoal(input, defendingTeam);
        const ShootingModelTuning shootingTuning;
        ShotOutcomeResult outcome = ShotOutcomeResolver{}.resolve(ShotOutcomeContext{
                pending->shotContext,
                pending->shotType,
                pending->shotExecution,
                pending->shotQuality,
                stepSeed(baseSeed, state, pending->sourcePlayerId ^ 0x9010ULL)
            });

        if (!outcome.onTarget) {
            clearChanceChain(chanceTracker);
            appendTrace(
                result,
                input.options.detail,
                state,
                outcome.kind == ShotOutcomeKind::Woodwork
                    ? MatchTraceKind::Woodwork
                    : MatchTraceKind::ShotOffTarget,
                pending->sourceTeamId,
                pending->sourcePlayerId,
                0,
                trajectory.actualTarget,
                trajectory.actualTarget);
            if (goalkeeper != nullptr) {
                setControlledBy(state, goalkeeper->playerId, goalkeeper->teamId, goalkeeper->position);
                ++teamStatsFor(result, goalkeeper->teamId).keeperClaims;
                ++playerStatsFor(result, goalkeeper->playerId, goalkeeper->teamId).keeperClaims;
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
        ++playerStatsFor(result, pending->sourcePlayerId, pending->sourceTeamId).shotsOnTarget;

        const PitchPoint savePoint = goalkeeperSaveContactPoint(
            trajectory,
            direction,
            goalkeeper,
            shootingTuning.flow);
        if (outcome.goal) {
            if (supportLifecycle.hadRecentSupportEvent(
                    pending->sourcePlayerId,
                    state.currentSecond)) {
                ++result.phaseDiagnostics.offBallSupportDiagnostics.goalsAfterSupportEvent;
            }
            const SimulationStepResult goalStep = applyLocalGoal(
                state,
                result,
                input,
                *pending,
                trajectory,
                defendingTeam.teamId,
                defendingTeam,
                chanceTracker);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToShot + goalStep.elapsedSeconds };
        }

        if (outcome.kind == ShotOutcomeKind::SavedHeld && goalkeeper != nullptr) {
            setControlledBy(state, goalkeeper->playerId, goalkeeper->teamId, savePoint);
            ++teamStatsFor(result, goalkeeper->teamId).keeperSavesHeld;
            ++playerStatsFor(result, goalkeeper->playerId, goalkeeper->teamId).keeperSavesHeld;
            clearChanceChain(chanceTracker);
            appendTrace(
                result,
                input.options.detail,
                state,
                MatchTraceKind::SaveHeld,
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
            ++teamStatsFor(result, goalkeeper->teamId).keeperSavesParried;
            ++playerStatsFor(result, goalkeeper->playerId, goalkeeper->teamId).keeperSavesParried;
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
                MatchTraceKind::SaveParried,
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
            markReboundChain(chanceTracker);
            return SimulationStepResult{ elapsedToShot };
        }

        clearChanceChain(chanceTracker);
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
        ChanceCreationTracker& chanceTracker,
        OffBallEventLifecycle& supportLifecycle) {
        if (!state.ball.trajectory) {
            setLooseBall(state, state.ball.position);
            clearChanceChain(chanceTracker);
            pending = std::nullopt;
            return SimulationStepResult{ 1.0 };
        }

        const BallTrajectory trajectory = *state.ball.trajectory;
        const double elapsedToArrival = remainingTrajectorySeconds(state, trajectory);
        if (pending && pending->kind != PendingBallKind::PlayerAction) {
            state.ball.position = PitchGeometry::clampToPitch(trajectory.actualTarget);
            setLooseBall(state, state.ball.position);
            state.possession.lastPossessionTeamId = pending->sourceTeamId;
            if (pending->kind == PendingBallKind::SavedRebound
                || pending->kind == PendingBallKind::BlockedDeflection) {
                markReboundChain(chanceTracker);
            } else {
                clearChanceChain(chanceTracker);
            }
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
            clearChanceChain(chanceTracker);
            pending = std::nullopt;
            return SimulationStepResult{ elapsedToArrival };
        }

        if (pending) {
            moveReceiverTowardPass(state, result, input, *pending, trajectory);
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
                    PassResolutionOutcome::MisplacedLoose,
                    state.ball.position,
                    trajectory.start,
                    trajectory.intendedTarget,
                    chanceTracker);
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
                    chanceTracker);
                pending = std::nullopt;
                return SimulationStepResult{ elapsedToArrival };
            }

            if (passResolution.outcome == PassResolutionOutcome::DeflectedLoose
                || passResolution.outcome == PassResolutionOutcome::MisplacedLoose
                || passResolution.outcome == PassResolutionOutcome::OutOfPlay) {
                if (passResolution.outcome == PassResolutionOutcome::DeflectedLoose
                    && arrivalDefender != nullptr) {
                    if (isAssignedGoalkeeper(input, arrivalDefender->teamId, arrivalDefender->playerId)) {
                        ++teamStatsFor(result, arrivalDefender->teamId).keeperSweeps;
                        ++playerStatsFor(
                            result,
                            arrivalDefender->playerId,
                            arrivalDefender->teamId).keeperSweeps;
                    } else {
                        ++teamStatsFor(result, arrivalDefender->teamId).passBlocks;
                        ++playerStatsFor(
                            result,
                            arrivalDefender->playerId,
                            arrivalDefender->teamId).passBlocks;
                    }
                }
                recordLoosePass(
                    state,
                    result,
                    input,
                    *pending,
                    passResolution.outcome,
                    state.ball.position,
                    trajectory.start,
                    trajectory.intendedTarget,
                    chanceTracker);
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
                MatchPlayerSimulationStats& passerStats = playerStatsFor(
                    result,
                    pending->sourcePlayerId,
                    pending->sourceTeamId);
                ++passerStats.passesCompleted;
                if (pending->actionType == BallCarrierActionType::ShortPass) {
                    ++passerStats.shortPassesCompleted;
                }
                const TeamSimState* receivingTeam = findTeamState(state, target->teamId);
                const AttackingDirection receivingDirection =
                    receivingTeam != nullptr
                        ? attackingDirectionForTeam(*receivingTeam)
                        : AttackingDirection::HomeToAway;
                recordReceptionInvolvement(
                    result,
                    assignedRoleForPlayer(input, target->teamId, target->playerId),
                    target->playerId,
                    state.ball.position,
                    receivingDirection,
                    pending->sourcePhase,
                    &supportLifecycle,
                    state.currentSecond);
                rememberCompletedPass(
                    chanceTracker,
                    state,
                    *pending,
                    target->playerId,
                    trajectory.start,
                    trajectory.intendedTarget,
                    state.ball.position,
                    receivingDirection,
                    pending->pressure);
            } else {
                markTurnoverChain(chanceTracker);
                recordPhaseTurnover(result, pending->sourcePhase);
                if (TeamSimState* sourceTeam = findTeamState(state, pending->sourceTeamId)) {
                    markFinalizingTurnover(result, *sourceTeam, pending->sourcePhase);
                }
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
                ++teamStatsFor(result, block.blockerTeamId).shotBlocks;
                ++playerStatsFor(result, block.blockerPlayerId, block.blockerTeamId).shotBlocks;
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
                    MatchTraceKind::BlockedShot,
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
                markReboundChain(chanceTracker);
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
                chanceTracker,
                supportLifecycle);
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
                        markTurnoverChain(chanceTracker);
                        recordPhaseTurnover(result, pending->sourcePhase);
                        if (isPassLike(pending->actionType)) {
                            if (isAssignedGoalkeeper(
                                    input,
                                    contest.cleanController->teamId,
                                    contest.cleanController->playerId)) {
                                ++teamStatsFor(result, contest.cleanController->teamId).keeperClaims;
                                ++playerStatsFor(
                                    result,
                                    contest.cleanController->playerId,
                                    contest.cleanController->teamId).keeperClaims;
                                ++teamStatsFor(result, pending->sourceTeamId).passesKeeperClaimed;
                            } else {
                                ++teamStatsFor(result, pending->sourceTeamId).passesIntercepted;
                                ++teamStatsFor(result, contest.cleanController->teamId).passInterceptions;
                                ++playerStatsFor(
                                    result,
                                    contest.cleanController->playerId,
                                    contest.cleanController->teamId).passInterceptions;
                                ++teamStatsFor(result, contest.cleanController->teamId).interceptions;
                                ++playerStatsFor(
                                    result,
                                    contest.cleanController->playerId,
                                    contest.cleanController->teamId).interceptions;
                            }
                        } else {
                            ++teamStatsFor(result, contest.cleanController->teamId).looseBallRecoveries;
                            ++playerStatsFor(
                                result,
                                contest.cleanController->playerId,
                                contest.cleanController->teamId).looseBallRecoveries;
                        }
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
                    const bool wasShotDeflection = pending->isShot;
                    if (isPassLike(pending->actionType)) {
                        ++teamStatsFor(result, pending->sourceTeamId).passesDeflected;
                        if (isAssignedGoalkeeper(input, candidate.teamId, candidate.playerId)) {
                            ++teamStatsFor(result, candidate.teamId).keeperSweeps;
                            ++playerStatsFor(result, candidate.playerId, candidate.teamId).keeperSweeps;
                        } else {
                            ++teamStatsFor(result, candidate.teamId).passBlocks;
                            ++playerStatsFor(result, candidate.playerId, candidate.teamId).passBlocks;
                        }
                    } else if (pending->isShot) {
                        ++teamStatsFor(result, candidate.teamId).shotBlocks;
                        ++playerStatsFor(result, candidate.playerId, candidate.teamId).shotBlocks;
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
                    if (wasShotDeflection) {
                        markReboundChain(chanceTracker);
                    } else {
                        clearChanceSource(chanceTracker);
                    }
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
                    recordPhaseTurnover(result, pending->sourcePhase);
                    setLooseBall(state, candidate.interceptionPoint);
                    clearChanceSource(chanceTracker);
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
                        chanceTracker,
                        supportLifecycle);
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
                chanceTracker,
                supportLifecycle);
        }

        setLooseBall(state, state.ball.position);
        clearChanceSource(chanceTracker);
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
        ChanceCreationTracker& chanceTracker) {
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
        ++teamStatsFor(result, best->teamId).looseBallRecoveries;
        ++playerStatsFor(result, best->playerId, best->teamId).looseBallRecoveries;
        if (chanceTracker.reboundSequenceActive) {
            clearChanceSource(chanceTracker);
        } else if (previousTeam != 0 && previousTeam != best->teamId) {
            markTurnoverChain(chanceTracker);
        } else if (previousTeam != 0 && previousTeam == best->teamId) {
            ++result.phaseDiagnostics.ignoredSameTeamLooseRegainPhaseSwitches;
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
    initializePhaseDiagnostics(result, state, input);

    const std::uint64_t baseSeed = baseSeedFor(input);
    const double deltaSeconds = deltaSecondsFor(input);

    MatchPhaseModel phaseModel;
    TeamShapeModel shapeModel;
    PlayerIntentResolver intentResolver;
    MovementResolver movementResolver;
    BallCarrierDecisionModel ballCarrierDecisionModel;
    ActionSelector actionSelector;
    BallTrajectoryBuilder trajectoryBuilder;
    InterceptionResolver interceptionResolver;
    ContestResolver contestResolver;
    OffBallSupportModel offBallSupportModel;
    OffBallEventLifecycle supportLifecycle;
    RestDefenseModel restDefenseModel;

    std::optional<ContestResolverResult> lastContestResult;
    std::optional<PendingBallAction> pending;
    ChanceCreationTracker chanceTracker;

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
        std::vector<PlayerShapeTarget> homeTargets = buildTargetsSafely(
            shapeModel,
            homeShapeContext,
            input.homeTeam.teamSheet);
        std::vector<PlayerShapeTarget> awayTargets = buildTargetsSafely(
            shapeModel,
            awayShapeContext,
            input.awayTeam.teamSheet);

        const TeamGameContext homeGameContext = phaseModel.buildTeamContext(
            state.homeTeam,
            state.awayTeam,
            state,
            homeTargets,
            awayTargets);
        const TeamGameContext awayGameContext = phaseModel.buildTeamContext(
            state.awayTeam,
            state.homeTeam,
            state,
            awayTargets,
            homeTargets);
        applyPhaseTransition(
            result,
            state.homeTeam,
            phaseModel.evaluateTeamPhase(homeGameContext),
            state.currentSecond,
            state.possession.actionDepth);
        applyPhaseTransition(
            result,
            state.awayTeam,
            phaseModel.evaluateTeamPhase(awayGameContext),
            state.currentSecond,
            state.possession.actionDepth);
        const std::vector<PlayerGameContext> homePlayerContexts = phaseModel.buildPlayerContexts(
            state.homeTeam,
            state.awayTeam,
            input.homeTeam.teamSheet,
            homeTargets,
            state.ball.position);
        const std::vector<PlayerGameContext> awayPlayerContexts = phaseModel.buildPlayerContexts(
            state.awayTeam,
            state.homeTeam,
            input.awayTeam.teamSheet,
            awayTargets,
            state.ball.position);

        recordSupportLifecycleResult(
            result,
            input,
            supportLifecycle.update(state, restDefenseModel));

        if (state.ball.controlState == BallControlState::Controlled) {
            const OffBallSupportModelResult homeSupport = offBallSupportModel.evaluate(OffBallSupportModelRequest{
                &state.homeTeam,
                &state.awayTeam,
                homeGameContext,
                homePlayerContexts,
                supportLifecycle.activeEventsForTeam(state.homeTeam.teamId),
                homeShapeContext.attackingDirection,
                state.currentSecond
            });
            recordSupportGenerationResult(result, input, homeSupport);
            recordSupportLifecycleResult(
                result,
                input,
                supportLifecycle.addEvents(homeSupport.events, state.currentSecond));

            const OffBallSupportModelResult awaySupport = offBallSupportModel.evaluate(OffBallSupportModelRequest{
                &state.awayTeam,
                &state.homeTeam,
                awayGameContext,
                awayPlayerContexts,
                supportLifecycle.activeEventsForTeam(state.awayTeam.teamId),
                awayShapeContext.attackingDirection,
                state.currentSecond
            });
            recordSupportGenerationResult(result, input, awaySupport);
            recordSupportLifecycleResult(
                result,
                input,
                supportLifecycle.addEvents(awaySupport.events, state.currentSecond));
            recordBothFullbacksAdvanced(result, input, supportLifecycle);
        }

        applySupportEventsToTargets(homeTargets, supportLifecycle);
        applySupportEventsToTargets(awayTargets, supportLifecycle);

        std::vector<ResolvedPlayerIntent> homeIntents = resolveTeamIntents(
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
        std::vector<ResolvedPlayerIntent> awayIntents = resolveTeamIntents(
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
        applySupportEventsToIntents(homeIntents, supportLifecycle);
        applySupportEventsToIntents(awayIntents, supportLifecycle);

        const PlayerId movementSkipPlayerId =
            state.ball.controlState == BallControlState::Controlled
                ? state.ball.carrierPlayerId
                : 0;

        moveTeamPlayers(
            movementResolver,
            result,
            state.homeTeam,
            homeIntents,
            deltaSeconds,
            movementSkipPlayerId);
        moveTeamPlayers(
            movementResolver,
            result,
            state.awayTeam,
            awayIntents,
            deltaSeconds,
            movementSkipPlayerId);
        recordSupportLifecycleResult(
            result,
            input,
            supportLifecycle.update(state, restDefenseModel));

        SimulationStepResult step;

        if (state.ball.controlState == BallControlState::Controlled) {
            const TeamShapeContext& carrierContext =
                state.ball.carrierTeamId == state.homeTeam.teamId
                    ? homeShapeContext
                    : awayShapeContext;
            const MatchTeamPhase carrierPhase =
                state.ball.carrierTeamId == state.homeTeam.teamId
                    ? actingTeamPhaseForDiagnostics(state.homeTeam, state)
                    : actingTeamPhaseForDiagnostics(state.awayTeam, state);
            const TeamGameContext& carrierTeamGameContext =
                state.ball.carrierTeamId == state.homeTeam.teamId
                    ? homeGameContext
                    : awayGameContext;
            const std::vector<PlayerGameContext>& carrierPlayerGameContexts =
                state.ball.carrierTeamId == state.homeTeam.teamId
                    ? homePlayerContexts
                    : awayPlayerContexts;
            step = executeControlledAction(
                state,
                result,
                input,
                carrierContext,
                carrierPhase,
                carrierTeamGameContext,
                carrierPlayerGameContexts,
                trajectoryBuilder,
                ballCarrierDecisionModel,
                actionSelector,
                pending,
                chanceTracker,
                supportLifecycle,
                restDefenseModel,
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
                chanceTracker,
                supportLifecycle);
        } else if (state.ball.controlState == BallControlState::Loose) {
            step = processLooseBall(state, result, input, chanceTracker);
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
        recordPhaseTime(result, state.homeTeam, homeGameContext, elapsedSeconds);
        recordPhaseTime(result, state.awayTeam, awayGameContext, elapsedSeconds);

        updateMeaningfulProgression(state);
        state.currentSecond = std::min(
            RegulationMatchSeconds,
            state.currentSecond + static_cast<int>(std::ceil(elapsedSeconds)));
    }

    result.simulatedSeconds = state.currentSecond;
    finalizePhaseDiagnostics(result, state);
    finalizePlayerMinutes(result, state.currentSecond);
    finalizePossessionShare(result, state);
    finalizePlayerRatings(result, input);
    result.report = MatchEngineReportAdapter{}.buildReport(input, result);
    return result;
}
