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
    Clearance
};

struct BallTrajectory {
    PitchPoint start;
    PitchPoint intendedTarget;
    PitchPoint actualTarget;
    double startSecond = 0.0;
    double arrivalSecond = 0.0;
    double speedMetersPerSecond = 0.0;
    BallTrajectoryType type = BallTrajectoryType::GroundPass;
};

enum class PlayerIntentType {
    HoldShape,
    MoveToSupport,
    MakeRunBehind,
    AttackNearPost,
    AttackFarPost,
    AttackCutbackZone,
    DropForPass,
    PressBallCarrier,
    ContainBallCarrier,
    TrackRunner,
    MarkOpponent,
    CoverSpace,
    ProtectGoalZone,
    BlockPassingLane,
    InterceptBallPath,
    AttemptTackle,
    RecoverToGoal,
    ClearDanger
};

enum class MatchActionType {
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
    PlayerIntentType type = PlayerIntentType::HoldShape;
    PitchPoint target;
    PlayerId relatedPlayerId = 0;
    double urgency = 0.0;
};

struct ActionCandidate {
    MatchActionType type = MatchActionType::Hold;
    PitchPoint intendedTarget;
    PlayerId targetPlayerId = 0;
    double tacticalScore = 0.0;
    double contextScore = 0.0;
    double mentalScore = 0.0;
    double skillConfidenceScore = 0.0;
    double pressurePenalty = 0.0;
    double finalScore = 0.0;
};
