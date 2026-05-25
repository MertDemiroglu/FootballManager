#pragma once

#include"fm/match_engine/PitchGeometry.h"
#include"fm/match_engine/TeamShapeModel.h"

class ShotQualityModel {
public:
    static double calculateOpenPlayXG(
        PitchPoint shotLocation,
        AttackingDirection attackingDirection,
        double pressure);
};
