#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/PitchGeometry.h"

#include<vector>

enum class MatchSimulationDetail {
    BackgroundSummary,
    WatchedMatch,
    DebugFullTrace
};

enum class MatchTraceKind {
    PossessionStart,
    Pass,
    ThroughBall,
    Carry,
    Dribble,
    LowCross,
    HighCross,
    Cutback,
    Shot,
    Save,
    Goal,
    Tackle,
    Interception,
    Foul,
    Card,
    SetPiece,
    Turnover,
    LooseBall,
    Clearance
};

struct PlayerMarkerSnapshot {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PitchPoint position;
    bool hasBall = false;
};

struct MatchTraceFrame {
    int second = 0;
    MatchTraceKind kind = MatchTraceKind::PossessionStart;
    TeamId teamId = 0;
    PlayerId primaryPlayerId = 0;
    PlayerId secondaryPlayerId = 0;
    PitchPoint ballPosition;
    PitchPoint ballTarget;
    std::vector<PlayerMarkerSnapshot> markers;
};

enum class BallTrajectoryType {
    GroundPass,
    ThroughBall,
    LowCross,
    HighCross,
    Cutback,
    Shot,
    Clearance,
    Deflection
};

enum class BallFlightProfile {
    Ground,
    Low,
    Medium,
    High,
    Lofted,
    Shot
};

struct BallTrajectory {
    PitchPoint start;
    PitchPoint intendedTarget;
    PitchPoint actualTarget;
    double startSecond = 0.0;
    double arrivalSecond = 0.0;
    double speedMetersPerSecond = 0.0;
    BallTrajectoryType type = BallTrajectoryType::GroundPass;
    BallFlightProfile flightProfile = BallFlightProfile::Ground;
    double apexHeightMeters = 0.0;
};

enum class PlayerIntentType {
    // Neutral / fallback
    None,

    // Attacking team off-ball intents
    HoldAttackingShape,
    MoveToSupport,
    DropForPass,
    MakeRunBehind,
    AttackNearPost,
    AttackFarPost,
    AttackCutbackZone,
    OccupyWidth,
    OccupyHalfSpace,

    // Defensive team intents
    HoldDefensiveShape,
    CoverSpace,
    MarkOpponent,
    PressBallCarrier,
    ContainBallCarrier,
    BlockPassingLane,
    InterceptBallPath,
    AttemptTackle,
    RecoverToGoal,
    ProtectGoalZone,

    // Loose / deflected ball reaction
    RecoverLooseBall,

    // Emergency
    ClearDanger
};

enum class BallCarrierActionType {
    ShortPass,
    BackPass,
    ThroughBall,
    SwitchPlay,
    Carry,
    Dribble,
    CutInside,
    LowCross,
    HighCross,
    Cutback,
    Shoot,
    Hold,
    Clear
};

enum class ContestType {
    PassingLaneInterception,
    GroundCrossInterception,
    ReceptionDuel,
    DribbleDuel,
    TackleAttempt,
    ShotBlock,
    GoalkeeperSave,
    AerialDuel,
    LooseBallRace
};

struct PlayerIntent {
    PlayerIntentType type = PlayerIntentType::None;
    PitchPoint target;
    PlayerId relatedPlayerId = 0;
    double urgency = 0.0;
};

struct ActionCandidate {
    BallCarrierActionType type = BallCarrierActionType::Hold;
    PitchPoint intendedTarget;
    PlayerId targetPlayerId = 0;
    double tacticalScore = 0.0;
    double contextScore = 0.0;
    double mentalScore = 0.0;
    double skillConfidenceScore = 0.0;
    double pressurePenalty = 0.0;
    double finalScore = 0.0;
};
