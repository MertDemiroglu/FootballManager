#pragma once

#include"fm/match_engine/ball/ShotTypes.h"

#include<cstdint>

struct ShotOutcomeContext {
    ShotContext shotContext;
    ShotType shotType = ShotType::ControlledFinish;
    ShotExecutionResult execution;
    ShotQualityResult quality;
    std::uint64_t seed = 0;
};

enum class ShotOutcomeKind {
    Goal,
    OffTarget,
    Blocked,
    SavedHeld,
    SavedRebound
};

struct ShotOutcomeResult {
    ShotOutcomeKind kind = ShotOutcomeKind::SavedHeld;
    bool onTarget = false;
    bool goal = false;
    bool rebound = false;
};

class ShotAccuracyResolver {
public:
    bool isOnTarget(const ShotOutcomeContext& context) const;
};

class GoalkeeperSaveResolver {
public:
    ShotOutcomeResult resolveOnTarget(const ShotOutcomeContext& context) const;
};

class ShotOutcomeResolver {
public:
    ShotOutcomeResult resolve(const ShotOutcomeContext& context) const;
};
