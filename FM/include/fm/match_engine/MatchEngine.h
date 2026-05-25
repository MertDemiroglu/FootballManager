#pragma once

#include"fm/match_engine/MatchEngineInput.h"
#include"fm/match_engine/MatchEngineResult.h"

class MatchEngine {
public:
    // Pure coordinate simulation entry point. World state is applied by callers.
    MatchEngineResult simulate(const MatchEngineInput& input) const;
};

bool hasValidTeams(const MatchEngineInput& input);
