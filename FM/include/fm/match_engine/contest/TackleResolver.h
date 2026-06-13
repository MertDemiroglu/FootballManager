#pragma once

#include"fm/match_engine/contest/PressureContext.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>

enum class DefensiveActionType {
    Contain,
    Press,
    Tackle,
    LaneBlock,
    Cover
};

struct TackleResolverRequest {
    PlayerIntentType defenderIntent = PlayerIntentType::None;
    PlayerAttributes defenderAttributes;
    PressureContext pressure;
    std::uint64_t seed = 0;
};

class TackleResolver {
public:
    DefensiveActionType selectAction(const TackleResolverRequest& request) const;
};
