#pragma once

#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/ball/ShotTypes.h"

class ShotQualityModel {
public:
    static double calculateOpenPlayXG(
        PitchPoint shotLocation,
        AttackingDirection attackingDirection,
        double pressure);

    ShotQualityResult evaluate(
        const ShotContext& context,
        ShotType shotType,
        const ShotExecutionResult& execution) const;
};
