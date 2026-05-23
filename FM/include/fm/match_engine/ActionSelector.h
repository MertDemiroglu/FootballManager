#pragma once

#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>
#include<optional>
#include<vector>

struct ActionSelectionRequest {
    std::vector<ActionCandidate> candidates;
    PlayerAttributes decisionMakerAttributes;
    std::uint64_t seed = 0;
};

struct ActionSelectionResult {
    std::optional<ActionCandidate> selected;
    std::vector<ActionCandidate> rankedCandidates;
};

class ActionSelector {
public:
    ActionSelectionResult select(const ActionSelectionRequest& request) const;
};
