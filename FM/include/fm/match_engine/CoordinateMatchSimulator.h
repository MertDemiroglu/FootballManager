#pragma once

#include"fm/match_engine/MatchEngineInput.h"
#include"fm/match_engine/MatchEngineResult.h"

class CoordinateMatchSimulator {
public:
    MatchEngineResult run(const MatchEngineInput& input) const;
};
