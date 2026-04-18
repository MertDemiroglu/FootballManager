#pragma once

#include"Formation.h"
#include"fm/common/Types.h"

#include<vector>

class Team;

struct TeamSheetSlotAssignment {
    FormationSlotRole slotRole = FormationSlotRole::Unknown;
    PlayerId playerId = 0;
};

struct TeamSheet {
    TeamId teamId = 0;
    CoachId coachId = 0;
    FormationId formation = FormationId::FourFourTwo;
    std::vector<TeamSheetSlotAssignment> startingAssignments;
    std::vector<PlayerId> startingPlayerIds;
};

void validateTeamSheet(const TeamSheet& teamSheet);
void validateTeamSheetForTeam(const TeamSheet& teamSheet, const Team& team);
