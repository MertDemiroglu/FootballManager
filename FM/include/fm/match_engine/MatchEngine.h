#pragma once

#include"fm/match_engine/MatchEngineInput.h"
#include"fm/match_engine/MatchEngineResult.h"

class MatchEngine {
public:
    // Skeleton only: the real coordinate simulation will arrive in later phases.
    // This class is intentionally not wired into the game loop yet.
    MatchEngineResult simulate(const MatchEngineInput& input) const;
};

bool hasValidTeams(const MatchEngineInput& input);
