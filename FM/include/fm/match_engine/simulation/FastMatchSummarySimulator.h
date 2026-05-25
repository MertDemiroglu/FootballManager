#pragma once

#include"fm/match_engine/MatchEngineInput.h"
#include"fm/match_engine/MatchEngineResult.h"

class FastMatchSummarySimulator {
public:
    MatchEngineResult run(const MatchEngineInput& input) const;
};
