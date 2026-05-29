#pragma once

#include<cstdint>

struct ShotOutcomeContext {
    double xg = 0.0;
    double placementQuality = 0.0;
    double targetDifficulty = 0.0;
    double shooterFinishing = 0.0;
    double shooterComposure = 0.0;
    double goalkeeperStrength = 0.0;
    double pressure = 0.0;
    std::uint64_t seed = 0;
};

enum class ShotOutcomeKind {
    OffTarget,
    Saved,
    Goal,
    Blocked
};

struct ShotOutcomeResult {
    ShotOutcomeKind kind = ShotOutcomeKind::Saved;
    bool onTarget = true;
    bool goal = false;
};

class ShotOutcomeResolver {
public:
    ShotOutcomeResult resolve(const ShotOutcomeContext& context) const;
};
