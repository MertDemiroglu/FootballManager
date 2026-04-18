#pragma once

#include"fm/roster/PlayerPosition.h"

enum class FormationSlotGroup {
    Goalkeeper,
    Defender,
    Midfielder,
    Forward,
    Unknown
};

using PlayerPositionBucket = FormationSlotGroup;

FormationSlotGroup mapPlayerPositionToFormationSlotGroup(PlayerPosition position);
FormationSlotGroup mapPlayerPositionToBucket(PlayerPosition position);
