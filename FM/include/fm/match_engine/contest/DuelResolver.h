#pragma once

#include"fm/match_engine/contest/TackleResolver.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>

enum class DuelOutcomeType {
    NoDuel,
    AttackerKeepsBall,
    AttackerBeatsDefender,
    DefenderWinsTackle,
    BallLoose,
    ForcedSideways,
    ForcedBackward
};

struct DuelContestant {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PitchPoint position;
    PlayerIntentType intentType = PlayerIntentType::None;
    PlayerAttributes attributes;
    int baseOverall = 0;
    double fatigue = 0.0;
};

struct DuelResolutionRequest {
    BallCarrierActionType actionType = BallCarrierActionType::Carry;
    DuelContestant attacker;
    DuelContestant defender;
    PressureContext pressure;
    PitchPoint start;
    PitchPoint intendedTarget;
    AttackingDirection direction = AttackingDirection::HomeToAway;
    std::uint64_t seed = 0;
};

struct DuelResolutionResult {
    bool duelOccurred = false;
    bool tackleAttempted = false;
    DefensiveActionType defensiveAction = DefensiveActionType::Contain;
    DuelOutcomeType outcome = DuelOutcomeType::NoDuel;
    PitchPoint resolvedTarget;
    PitchPoint loosePoint;
    double addedExecutionPressure = 0.0;
};

class DuelResolver {
public:
    DuelResolutionResult resolve(const DuelResolutionRequest& request) const;
};
