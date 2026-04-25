#pragma once

#include"Formation.h"

#include<cstddef>
#include<optional>

struct FormationPitchCoordinate {
    double x = 0.5;
    double y = 0.5;
    int displayOrder = 0;
};

std::optional<FormationPitchCoordinate> getFormationPitchCoordinate(
    FormationId formationId,
    std::size_t slotIndex,
    FormationSlotRole slotRole);

FormationPitchCoordinate getFallbackFormationPitchCoordinate(
    std::size_t slotIndex,
    std::size_t slotCount,
    FormationSlotRole slotRole);
