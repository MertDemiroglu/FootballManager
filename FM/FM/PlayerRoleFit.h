#pragma once

#include "FormationSlotRole.h"
#include "PlayerPosition.h"

class Footballer;

enum class RoleFitTier {
    Natural,
    Close,
    Adapted,
    Emergency,
    Invalid
};

RoleFitTier getRoleFitTier(PlayerPosition playerPosition, FormationSlotRole slotRole);
double getRoleFitMultiplier(RoleFitTier tier);
double getRoleFitMultiplier(PlayerPosition playerPosition, FormationSlotRole slotRole);
bool isRoleFitUsable(PlayerPosition playerPosition, FormationSlotRole slotRole);
double calculateEffectivePowerForSlot(const Footballer& player, FormationSlotRole slotRole);
