#pragma once

#include "TeamSheet.h"

class Team;

class TeamSelectionService {
public:
    TeamSheet buildTeamSheet(const Team& team) const;
};
