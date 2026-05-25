#pragma once

#include"fm/match_engine/MatchEngineTypes.h"

enum class BallCarrierActionCategory {
    Pass,
    Carry,
    Shoot
};

enum class PassOptionKind {
    SafePass,
    BackPass,
    ProgressivePass,
    SwitchPlay,
    ThroughBall,
    Cross,
    Cutback
};

enum class CarryOptionKind {
    SafeCarry,
    ProgressiveCarry,
    Dribble
};

enum class ShotOptionKind {
    OpenPlayShot
};

struct PassDecisionOption {
    PassOptionKind kind = PassOptionKind::SafePass;
    BallCarrierActionType actionType = BallCarrierActionType::ShortPass;
};

struct CarryDecisionOption {
    CarryOptionKind kind = CarryOptionKind::SafeCarry;
    BallCarrierActionType actionType = BallCarrierActionType::Carry;
};

struct ShotDecisionOption {
    ShotOptionKind kind = ShotOptionKind::OpenPlayShot;
    BallCarrierActionType actionType = BallCarrierActionType::Shoot;
};
