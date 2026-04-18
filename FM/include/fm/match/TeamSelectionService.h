#pragma once

#include"fm/match/TeamSheet.h"

class Team;

class TeamSelectionService {
public:
    TeamSheet buildTeamSheet(const Team& team) const;
};
