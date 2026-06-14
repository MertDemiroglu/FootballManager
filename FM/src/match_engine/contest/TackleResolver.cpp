#include"fm/match_engine/contest/TackleResolver.h"

#include"../DeterministicRandom.h"

#include<algorithm>

namespace {
    double clampedAttribute(int value) {
        return std::clamp(static_cast<double>(value), 0.0, 100.0);
    }

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }
}

DefensiveActionType TackleResolver::selectAction(const TackleResolverRequest& request) const {
    const PlayerIntentType intent = request.defenderIntent;
    if (intent == PlayerIntentType::BlockPassingLane
        || intent == PlayerIntentType::InterceptBallPath
        || request.pressure.defenderBetweenBallAndPath) {
        return DefensiveActionType::LaneBlock;
    }
    if (intent == PlayerIntentType::CoverSpace
        || intent == PlayerIntentType::RecoverToGoal
        || intent == PlayerIntentType::ProtectGoalZone) {
        return DefensiveActionType::Cover;
    }

    const PlayerAttributes& attributes = request.defenderAttributes;
    double commit = clampedAttribute(attributes.technical.tackling) * 0.34
        + clampedAttribute(attributes.mental.aggression) * 0.24
        + clampedAttribute(attributes.mental.decisions) * 0.16
        + clampedAttribute(attributes.physical.strength) * 0.12
        + request.pressure.pressureStrength * 0.07;
    commit += request.pressure.defenderBetweenBallAndGoal ? 8.0 : 0.0;
    commit += zonePressureBonus(request.pressure.dangerZone) * 0.25;
    commit -= request.pressure.closestOutfieldDefenderDistance * 8.5;
    if (intent == PlayerIntentType::AttemptTackle) {
        commit += 12.0;
    } else if (intent == PlayerIntentType::PressBallCarrier) {
        commit += 4.0;
    } else if (intent == PlayerIntentType::ContainBallCarrier) {
        commit -= 14.0;
    }

    const double roll = matchEngineDeterministicUnitInterval(request.seed ^ 0xcf8840a723f1d07bULL) * 100.0;
    if (roll < clampDouble(commit, 5.0, 48.0)) {
        return DefensiveActionType::Tackle;
    }
    return request.pressure.pressureStrength >= 36.0
        ? DefensiveActionType::Press
        : DefensiveActionType::Contain;
}
