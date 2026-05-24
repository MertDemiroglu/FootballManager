#pragma once

#include"fm/match_engine/MatchEngineInput.h"
#include"fm/match_engine/MatchEngineResult.h"

class CoordinateSimulationPrototype {
public:
    MatchEngineResult run(const MatchEngineInput& input) const;
};
