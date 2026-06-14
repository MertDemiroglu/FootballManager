#pragma once

#include"fm/match/TacticalSetup.h"
#include"fm/roster/Formation.h"
#include"fm/common/Types.h"

#include<cstddef>
#include<vector>

class Team;

constexpr std::size_t kMaxSubstituteCount = 10;

struct TeamSheetSlotAssignment {
    std::size_t slotIndex = 0;
    FormationSlotRole slotRole = FormationSlotRole::Unknown;
    PlayerId playerId = 0;
};

struct TeamSheet {
    TeamId teamId = 0;
    CoachId coachId = 0;
    FormationId formation = FormationId::FourThreeThree;
    std::vector<TeamSheetSlotAssignment> startingAssignments;
    std::vector<PlayerId> startingPlayerIds;
    std::vector<PlayerId> substitutePlayerIds;
    TacticalSetup tacticalSetup;
};

void validateTeamSheet(const TeamSheet& teamSheet);
void validateTeamSheetForTeam(const TeamSheet& teamSheet, const Team& team);
