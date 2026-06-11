#pragma once

#include"fm/common/Types.h"
#include"fm/match/MatchReport.h"
#include"fm/match_engine/MatchEngineTypes.h"

#include<optional>
#include<vector>

enum class MatchGoalSourceCategory {
    AssistedFinalBall,
    AssistedSimplePass,
    CarryAfterReceive,
    SoloCarry,
    Rebound,
    Turnover,
    Unknown
};

struct MatchTeamSimulationStats {
    TeamId teamId = 0;
    int goals = 0;
    int shots = 0;
    int shotsOnTarget = 0;
    int passesAttempted = 0;
    int passesCompleted = 0;
    int tacklesAttempted = 0;
    int tacklesWon = 0;
    int interceptions = 0;
    int passesIntercepted = 0;
    int passesDeflected = 0;
    int passesLoose = 0;
    int passesReceiverOutOfRange = 0;
    int fouls = 0;
    int corners = 0;
    int yellowCards = 0;
    int redCards = 0;
    double possessionShare = 0.0;
    double expectedGoals = 0.0;
    double rawExpectedGoals = 0.0;
    double preShotExpectedGoals = 0.0;
    double keeperFacingExpectedGoals = 0.0;
    double blockedExpectedGoals = 0.0;
    int assistedGoals = 0;
    int unassistedOpenPlayGoals = 0;
    int reboundGoals = 0;
    int transitionGoals = 0;
    int assistedShots = 0;
    int soloCarryShots = 0;
    int reboundShots = 0;
    int turnoverShots = 0;
    int finalBallShots = 0;
    int cutbackShots = 0;
    int throughBallShots = 0;
    int lowCrossShots = 0;
};

struct MatchPlayerSimulationStats {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    int minutesPlayed = 0;
    int goals = 0;
    int assists = 0;
    int shots = 0;
    int shotsOnTarget = 0;
    int passesAttempted = 0;
    int passesCompleted = 0;
    int shortPassesAttempted = 0;
    int shortPassesCompleted = 0;
    int throughBalls = 0;
    int lowCrosses = 0;
    int cutbacks = 0;
    int finalBalls = 0;
    int assistedShotsReceived = 0;
    int finalBallShotsReceived = 0;
    int carryTraces = 0;
    int dribbleTraces = 0;
    int progressiveCarries = 0;
    int tackles = 0;
    int interceptions = 0;
    double expectedGoals = 0.0;
    double preShotExpectedGoals = 0.0;
    double totalDistanceCoveredMeters = 0.0;
    double onBallCarryDistanceMeters = 0.0;
    double offBallMovementDistanceMeters = 0.0;
    int fouls = 0;
    int yellowCards = 0;
    int redCards = 0;
    double rating = 6.0;
};

struct MatchGoalChainDiagnostic {
    int minute = 0;
    TeamId teamId = 0;
    PlayerId scorerPlayerId = 0;
    PlayerId assistPlayerId = 0;
    BallCarrierActionType sourceActionType = BallCarrierActionType::Hold;
    MatchGoalSourceCategory sourceCategory = MatchGoalSourceCategory::Unknown;
    double shotDistanceMeters = 0.0;
    double preShotXG = 0.0;
    double effectiveXG = 0.0;
    bool followedControlledCarryAfterReceive = false;
    bool finalBallThroughBall = false;
    bool finalBallLowCross = false;
    bool finalBallCutback = false;
    bool finalBallHighCross = false;
    bool finalBallSimplePass = false;
};

struct MatchEngineResult {
    std::optional<MatchReport> report;
    MatchTeamSimulationStats homeStats;
    MatchTeamSimulationStats awayStats;
    std::vector<MatchPlayerSimulationStats> playerStats;
    std::vector<MatchEventRecord> events;
    std::vector<MatchGoalChainDiagnostic> goalChains;
    std::vector<MatchTraceFrame> traceFrames;
    int simulatedSeconds = 0;
};
