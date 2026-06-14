#pragma once

enum class OffBallEventType {
    None,
    OverlapRun,
    UnderlapRun,
    CutInsideRun,
    FarPostRun,
    NearPostRun,
    PenaltySpotRun,
    EdgeOfBoxSupport,
    HalfSpaceSupport,
    WideSupport,
    BackPassSupport,
    RestDefenseHold,
    CounterForwardRun,
    DefensiveRecoveryRun
};

constexpr int OffBallEventTypeCount = 14;

enum class SupportRegionLane {
    Any,
    Left,
    Right,
    Center,
    HalfSpaceLeft,
    HalfSpaceRight
};

enum class SupportRegionDepth {
    Any,
    Edge,
    Box,
    Byline,
    FarPost,
    NearPost,
    BackSupport
};

enum class OffBallEventCompletionReason {
    None,
    ReachedRegion,
    PossessionLoss,
    Shot,
    BallOut,
    OpponentControl,
    PhaseChange,
    Timeout,
    BecameBallCarrier,
    RestDefenseUnsafe,
    Replaced
};

inline int offBallEventTypeIndex(OffBallEventType type) {
    return static_cast<int>(type);
}

inline const char* offBallEventTypeName(OffBallEventType type) {
    switch (type) {
    case OffBallEventType::None:
        return "None";
    case OffBallEventType::OverlapRun:
        return "OverlapRun";
    case OffBallEventType::UnderlapRun:
        return "UnderlapRun";
    case OffBallEventType::CutInsideRun:
        return "CutInsideRun";
    case OffBallEventType::FarPostRun:
        return "FarPostRun";
    case OffBallEventType::NearPostRun:
        return "NearPostRun";
    case OffBallEventType::PenaltySpotRun:
        return "PenaltySpotRun";
    case OffBallEventType::EdgeOfBoxSupport:
        return "EdgeOfBoxSupport";
    case OffBallEventType::HalfSpaceSupport:
        return "HalfSpaceSupport";
    case OffBallEventType::WideSupport:
        return "WideSupport";
    case OffBallEventType::BackPassSupport:
        return "BackPassSupport";
    case OffBallEventType::RestDefenseHold:
        return "RestDefenseHold";
    case OffBallEventType::CounterForwardRun:
        return "CounterForwardRun";
    case OffBallEventType::DefensiveRecoveryRun:
        return "DefensiveRecoveryRun";
    }
    return "Unknown";
}

inline const char* offBallCompletionReasonName(OffBallEventCompletionReason reason) {
    switch (reason) {
    case OffBallEventCompletionReason::None:
        return "None";
    case OffBallEventCompletionReason::ReachedRegion:
        return "ReachedRegion";
    case OffBallEventCompletionReason::PossessionLoss:
        return "PossessionLoss";
    case OffBallEventCompletionReason::Shot:
        return "Shot";
    case OffBallEventCompletionReason::BallOut:
        return "BallOut";
    case OffBallEventCompletionReason::OpponentControl:
        return "OpponentControl";
    case OffBallEventCompletionReason::PhaseChange:
        return "PhaseChange";
    case OffBallEventCompletionReason::Timeout:
        return "Timeout";
    case OffBallEventCompletionReason::BecameBallCarrier:
        return "BecameBallCarrier";
    case OffBallEventCompletionReason::RestDefenseUnsafe:
        return "RestDefenseUnsafe";
    case OffBallEventCompletionReason::Replaced:
        return "Replaced";
    }
    return "Unknown";
}

inline bool isRestDefenseEvent(OffBallEventType type) {
    return type == OffBallEventType::RestDefenseHold
        || type == OffBallEventType::DefensiveRecoveryRun;
}

inline bool isAttackingSupportEvent(OffBallEventType type) {
    return type != OffBallEventType::None
        && type != OffBallEventType::RestDefenseHold
        && type != OffBallEventType::DefensiveRecoveryRun;
}
