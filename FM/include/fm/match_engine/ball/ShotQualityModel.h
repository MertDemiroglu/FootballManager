#pragma once

#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

class ShotQualityModel {
public:
    static double calculateOpenPlayXG(
        PitchPoint shotLocation,
        AttackingDirection attackingDirection,
        double pressure);
};
