#pragma once

#include "Formation.h"
#include "Types.h"

#include <vector>

class Team;

struct TeamSheet {
    TeamId teamId = 0;
    CoachId coachId = 0;
    FormationId formation = FormationId::FourFourTwo;
    std::vector<PlayerId> startingPlayerIds;
};

void validateTeamSheet(const TeamSheet& teamSheet);
void validateTeamSheetForTeam(const TeamSheet& teamSheet, const Team& team);
