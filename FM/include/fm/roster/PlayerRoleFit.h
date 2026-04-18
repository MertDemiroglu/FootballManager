#pragma once

#include"FormationSlotRole.h"
#include"fm/roster/PlayerPosition.h"

class Footballer;

enum class RoleFitTier {
    Natural,
    Close,
    Adapted,
    Emergency,
    SevereMismatch,
    Invalid
};

RoleFitTier getRoleFitTier(PlayerPosition playerPosition, FormationSlotRole slotRole);
double getRoleFitMultiplier(RoleFitTier tier);
double getRoleFitMultiplier(PlayerPosition playerPosition, FormationSlotRole slotRole);
bool isRoleFitUsable(PlayerPosition playerPosition, FormationSlotRole slotRole);
double calculateEffectivePowerForSlot(const Footballer& player, FormationSlotRole slotRole);
int getRoleFitTierScore(RoleFitTier tier);
