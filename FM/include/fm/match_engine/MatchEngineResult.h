#pragma once

#include"fm/common/Types.h"
#include"fm/match/MatchReport.h"
#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/match_engine/decision/PhaseDecisionContext.h"
#include"fm/match_engine/phase/MatchPhaseTypes.h"

#include<array>
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
    int passInterceptions = 0;
    int looseBallRecoveries = 0;
    int keeperClaims = 0;
    int keeperSavesHeld = 0;
    int keeperSavesParried = 0;
    int keeperSweeps = 0;
    int keeperOneOnOnes = 0;
    int keeperSmothers = 0;
    int passBlocks = 0;
    int shotBlocks = 0;
    int clearances = 0;
    int pressures = 0;
    int duels = 0;
    int carryUnderPressure = 0;
    int tacklesLost = 0;
    int dispossessionsForced = 0;
    int forcedSideways = 0;
    int forcedBackward = 0;
    int defenderWinsTackle = 0;
    int ballLooseFromDuel = 0;
    int attackerKeepsUnderPressure = 0;
    int attackerBeatsDefender = 0;
    int dribblesAttempted = 0;
    int dribblesWon = 0;
    int dribblesLost = 0;
    int passesIntercepted = 0;
    int passesDeflected = 0;
    int passesLoose = 0;
    int passesOutOfPlay = 0;
    int passesKeeperClaimed = 0;
    int passesFailedOther = 0;
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
    int passInterceptions = 0;
    int looseBallRecoveries = 0;
    int keeperClaims = 0;
    int keeperSavesHeld = 0;
    int keeperSavesParried = 0;
    int keeperSweeps = 0;
    int keeperOneOnOnes = 0;
    int keeperSmothers = 0;
    int passBlocks = 0;
    int shotBlocks = 0;
    int clearances = 0;
    int pressures = 0;
    int duels = 0;
    int tackleAttempts = 0;
    int tacklesWon = 0;
    int tacklesLost = 0;
    int dispossessionsForced = 0;
    int carryUnderPressure = 0;
    int forcedSideways = 0;
    int forcedBackward = 0;
    int defenderWinsTackle = 0;
    int ballLooseFromDuel = 0;
    int attackerKeepsUnderPressure = 0;
    int attackerBeatsDefender = 0;
    int dribblesAttempted = 0;
    int dribblesWon = 0;
    int dribblesLost = 0;
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
    double keeperFacingXG = 0.0;
    double effectiveXG = 0.0;
    double shotPressure = 0.0;
    double closestDefenderDistance = 0.0;
    double closestOutfieldDefenderDistance = -1.0;
    const char* closestDefenderRole = "none";
    double carryAfterReceiveMeters = 0.0;
    int touchesAfterReceive = 0;
    int defendersBeatenAfterReceive = 0;
    bool assistRetained = false;
    bool followedControlledCarryAfterReceive = false;
    bool finalBallThroughBall = false;
    bool finalBallLowCross = false;
    bool finalBallCutback = false;
    bool finalBallHighCross = false;
    bool finalBallSimplePass = false;
    bool rebound = false;
    bool transition = false;
    bool turnover = false;
    const char* assistNoneReason = "Unknown";
};

struct MatchTeamPhaseDiagnostic {
    TeamId teamId = 0;
    MatchTeamPhase finalPhase = MatchTeamPhase::SettledDefense;
    int phaseSwitchCount = 0;
    double longestSinglePhaseSeconds = 0.0;
    std::array<double, MatchTeamPhaseCount> phaseTimeSeconds{};
    std::array<int, MatchTeamPhaseCount> phaseEntries{};
};

enum class MatchDiagnosticRoleBucket {
    CenterBack,
    FullbackWingback,
    CentralMidfield,
    Winger,
    Striker,
    GoalkeeperOrOther
};

constexpr int MatchDiagnosticRoleBucketCount = 6;
constexpr int MatchDiagnosticActionTypeCount = 13;

struct MatchShotCreationDiagnostics {
    std::array<int, MatchDiagnosticRoleBucketCount> shotsCreatedByRole{};
    std::array<double, MatchDiagnosticRoleBucketCount> xGCreatedByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> goalsCreatedByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> shotAssistsByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> keyPassesByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> finalBallShotAssistsByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> simplePassShotAssistsByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> lowCrossShotAssistsByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> cutbackShotAssistsByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> throughBallShotAssistsByRole{};
    std::array<int, MatchDiagnosticRoleBucketCount> highCrossShotAssistsByRole{};
};

struct MatchShotReceiverDiagnostics {
    std::array<
        std::array<int, MatchDiagnosticActionTypeCount>,
        MatchDiagnosticRoleBucketCount> shotsByShooterRoleAndSourceAction{};
    std::array<
        std::array<double, MatchDiagnosticActionTypeCount>,
        MatchDiagnosticRoleBucketCount> xGByShooterRoleAndSourceAction{};
    std::array<
        std::array<int, MatchDiagnosticActionTypeCount>,
        MatchDiagnosticRoleBucketCount> goalsByShooterRoleAndSourceAction{};
};

struct MatchWidePlayerInvolvementDiagnostic {
    int actions = 0;
    int receptions = 0;
    int finalThirdReceptions = 0;
    int wideFinalThirdReceptions = 0;
    int halfSpaceReceptions = 0;
    int boxReceptions = 0;
    int penaltyAreaReceptions = 0;
    int boxCarries = 0;
    int cutInHalfSpaceCarries = 0;
    int cutInsideActions = 0;
    int carriesEndingInBox = 0;
    int shots = 0;
    double xG = 0.0;
    int shotAssists = 0;
    int finalBalls = 0;
    int lowCrosses = 0;
    int cutbacks = 0;
    int throughBalls = 0;
    int successfulFinalThirdEntries = 0;
};

struct MatchWingerInvolvementDiagnostics {
    MatchWidePlayerInvolvementDiagnostic left;
    MatchWidePlayerInvolvementDiagnostic right;
};

struct MatchFullbackSupportDiagnostics {
    int receptions = 0;
    int finalThirdReceptions = 0;
    int wideFinalThirdReceptions = 0;
    int advancedWideReceptions = 0;
    int lowCrosses = 0;
    int cutbacks = 0;
    int finalBalls = 0;
    int shotAssists = 0;
    double xGCreated = 0.0;
    int carriesIntoFinalThird = 0;
    int carriesIntoBox = 0;
    int shots = 0;
};

struct MatchFinalizingQualityDiagnostics {
    int finalizingEntries = 0;
    double finalizingDurationSeconds = 0.0;
    int finalizingPossessionsEndingInShot = 0;
    int finalizingPossessionsEndingInTurnover = 0;
    int finalizingPossessionsRecycledToBuildUp = 0;
    int finalizingFinalBalls = 0;
    int finalizingFinalBallShotAssists = 0;
    int finalizingWingerReceptions = 0;
    int finalizingFullbackReceptions = 0;
    int finalizingCMEdgeReceptions = 0;
    int finalizingSTReceptions = 0;
    int finalizingNonSTShots = 0;
    double finalizingNonSTxG = 0.0;
};

struct MatchBuildUpToFinalizingQualityDiagnostics {
    int buildUpToFinalizingEntries = 0;
    int passesBeforeFirstFinalizingActionTotal = 0;
    int passesBeforeFirstFinalizingActionSamples = 0;
    std::array<int, MatchDiagnosticActionTypeCount> firstFinalizingActionType{};
    int firstThreeShots = 0;
    int firstThreeFinalBalls = 0;
    int firstThreeRecycleActions = 0;
    int firstThreeTurnovers = 0;
    double finalizingPositionChainXG = 0.0;
};

struct MatchPhaseDiagnostics {
    std::array<double, MatchTeamPhaseCount> phaseTimeSeconds{};
    std::array<int, MatchTeamPhaseCount> phaseEntries{};
    std::array<int, MatchTeamPhaseCount> passesByPhase{};
    std::array<int, MatchTeamPhaseCount> carriesByPhase{};
    std::array<int, MatchTeamPhaseCount> dribblesByPhase{};
    std::array<int, MatchTeamPhaseCount> shotsByPhase{};
    std::array<int, MatchTeamPhaseCount> goalsByPhase{};
    std::array<double, MatchTeamPhaseCount> xGByPhase{};
    std::array<int, MatchTeamPhaseCount> finalBallsByPhase{};
    std::array<int, MatchTeamPhaseCount> turnoversByPhase{};

    int counterEntries = 0;
    int validCounterEntries = 0;
    int counterRejectedNoCleanRegain = 0;
    int counterRejectedNoForwardLane = 0;
    int counterRejectedNoRunner = 0;
    int counterRejectedOpponentRecovered = 0;
    int counterRejectedSettledPossession = 0;
    int counterShots = 0;
    int counterGoals = 0;
    double counterXG = 0.0;
    int counterExpiredNoForwardLane = 0;
    int counterExpiredDefenseRecovered = 0;
    int counterExpiredForcedBackwardOrSideways = 0;
    int counterExpiredRecycledToBuildUp = 0;
    int defensiveTransitionEntries = 0;
    int settledDefenseEntries = 0;
    int phaseSwitchCount = 0;
    int buildUpToFinalizingSwitches = 0;
    int finalizingToBuildUpSwitches = 0;
    int anyToCounterSwitches = 0;
    int anyToDefensiveTransitionSwitches = 0;
    int defensiveTransitionToSettledDefenseSwitches = 0;
    int looseBallPhaseHolds = 0;
    int ignoredSameTeamLooseRegainPhaseSwitches = 0;
    double counterDurationTotalSeconds = 0.0;
    std::vector<double> counterDurationSamplesSeconds;

    double teamShapeSettledSamples = 0.0;
    double teamShapeSettledCount = 0.0;
    double opponentShapeSettledCount = 0.0;
    int restDefenseStableCount = 0;
    int restDefenseBrokenCount = 0;
    double openForwardLaneTotal = 0.0;
    double openWideLaneLeftTotal = 0.0;
    double openWideLaneRightTotal = 0.0;
    double centralSpaceAvailableTotal = 0.0;
    double ballFlankLeftPossessionSeconds = 0.0;
    double ballFlankCenterPossessionSeconds = 0.0;
    double ballFlankRightPossessionSeconds = 0.0;

    bool defaultFormationFourThreeThree = false;
    PhaseDecisionDiagnostics phaseDecisionDiagnostics;
    MatchShotCreationDiagnostics shotCreationDiagnostics;
    MatchShotReceiverDiagnostics shotReceiverDiagnostics;
    MatchWingerInvolvementDiagnostics wingerInvolvementDiagnostics;
    MatchFullbackSupportDiagnostics fullbackSupportDiagnostics;
    MatchFinalizingQualityDiagnostics finalizingQualityDiagnostics;
    MatchBuildUpToFinalizingQualityDiagnostics buildUpToFinalizingQualityDiagnostics;
    std::vector<MatchTeamPhaseDiagnostic> teamDiagnostics;
};

struct MatchEngineResult {
    std::optional<MatchReport> report;
    MatchTeamSimulationStats homeStats;
    MatchTeamSimulationStats awayStats;
    std::vector<MatchPlayerSimulationStats> playerStats;
    std::vector<MatchEventRecord> events;
    std::vector<MatchGoalChainDiagnostic> goalChains;
    std::vector<MatchTraceFrame> traceFrames;
    MatchPhaseDiagnostics phaseDiagnostics;
    int simulatedSeconds = 0;
};
