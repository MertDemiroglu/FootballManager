#pragma once

#include "fm/presentation/PresentationDtos.h"

class Team;

class TeamPresentationBuilder {
public:
    TeamVisualDto buildTeamVisual(const Team* team) const;
};
