#pragma once

#include "fm/roster/PlayerAttributes.h"
#include "fm/roster/PlayerPosition.h"

class PlayerOverallCalculator {
public:
    static int calculateOverall(const PlayerAttributes& attributes, PlayerPosition position);
};
