#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/ball/BallTrajectoryTypes.h"
#include"fm/match_engine/contest/ContestTypes.h"
#include"fm/match_engine/geometry/PitchGeometry.h"

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
    ShotOffTarget,
    Woodwork,
    BlockedShot,
    Save,
    SaveHeld,
    SaveParried,
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
    ReceivePass,
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
