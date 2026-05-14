#pragma once

#include "fm/match/TacticalSetup.h"
#include "fm/roster/Formation.h"

struct TacticalPreferences {
    FormationId preferredFormation = FormationId::FourFourTwo;
    TeamMentality preferredMentality = TeamMentality::Balanced;
    TeamTempo preferredTempo = TeamTempo::Normal;
};
