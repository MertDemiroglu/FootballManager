#include"fm/match_engine/ball/ShotOutcomeResolver.h"

#include"../DeterministicRandom.h"

#include<algorithm>

namespace {
    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }
}

ShotOutcomeResult ShotOutcomeResolver::resolve(const ShotOutcomeContext& context) const {
    const double placementModifier = clampDouble(
        0.96 + (context.placementQuality - context.targetDifficulty) / 420.0,
        0.84,
        1.14);
    const double shooterModifier = clampDouble(
        0.98
            + (context.shooterFinishing - 60.0) / 260.0
            + (context.shooterComposure - 60.0) / 330.0,
        0.85,
        1.18);
    const double goalkeeperModifier = clampDouble(
        1.02 - ((context.goalkeeperStrength - 60.0) / 230.0),
        0.78,
        1.16);
    const double pressureModifier = clampDouble(1.0 - (context.pressure / 650.0), 0.88, 1.0);
    const double goalProbability = clampDouble(
        context.xg * placementModifier * shooterModifier * goalkeeperModifier * pressureModifier,
        0.008,
        0.42);

    const bool goal = matchEngineDeterministicUnitInterval(context.seed) < goalProbability;
    return ShotOutcomeResult{
        goal ? ShotOutcomeKind::Goal : ShotOutcomeKind::Saved,
        true,
        goal
    };
}
