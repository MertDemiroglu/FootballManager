#pragma once

#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/offside/OffsideTypes.h"

class OffsideLineModel {
public:
    OffsideLineResult evaluate(
        const TeamSimState& defendingTeam,
        PitchPoint ballPosition,
        AttackingDirection attackingDirection) const;
};

