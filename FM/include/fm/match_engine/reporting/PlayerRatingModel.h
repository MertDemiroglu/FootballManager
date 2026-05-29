#pragma once

#include"fm/match_engine/MatchEngineResult.h"
#include"fm/roster/FormationSlotRole.h"

struct PlayerRatingContext {
    MatchPlayerSimulationStats stats;
    FormationSlotRole role = FormationSlotRole::Unknown;
    int teamGoalsFor = 0;
    int teamGoalsAgainst = 0;
};

class PlayerRatingModel {
public:
    double calculate(const PlayerRatingContext& context) const;
};
