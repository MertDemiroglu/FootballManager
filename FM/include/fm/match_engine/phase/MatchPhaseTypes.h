#pragma once

#include<array>
#include<string>

enum class MatchTeamPhase {
    BuildUp,
    FinalizingPosition,
    CounterAttack,
    DefensiveTransition,
    SettledDefense
};

enum class BallZone {
    DefensiveThird,
    MiddleThird,
    AttackingHalf,
    FinalThird
};

enum class BallFlank {
    Left,
    Center,
    Right
};

constexpr int MatchTeamPhaseCount = 5;

inline int matchTeamPhaseIndex(MatchTeamPhase phase) {
    return static_cast<int>(phase);
}

inline const char* matchTeamPhaseName(MatchTeamPhase phase) {
    switch (phase) {
    case MatchTeamPhase::BuildUp:
        return "BuildUp";
    case MatchTeamPhase::FinalizingPosition:
        return "FinalizingPosition";
    case MatchTeamPhase::CounterAttack:
        return "CounterAttack";
    case MatchTeamPhase::DefensiveTransition:
        return "DefensiveTransition";
    case MatchTeamPhase::SettledDefense:
        return "SettledDefense";
    }
    return "Unknown";
}

inline bool isInPossessionPhase(MatchTeamPhase phase) {
    return phase == MatchTeamPhase::BuildUp
        || phase == MatchTeamPhase::FinalizingPosition
        || phase == MatchTeamPhase::CounterAttack;
}

inline std::array<MatchTeamPhase, MatchTeamPhaseCount> allMatchTeamPhases() {
    return {
        MatchTeamPhase::BuildUp,
        MatchTeamPhase::FinalizingPosition,
        MatchTeamPhase::CounterAttack,
        MatchTeamPhase::DefensiveTransition,
        MatchTeamPhase::SettledDefense
    };
}

