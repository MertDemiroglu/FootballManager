#pragma once

#include"fm/match_engine/movement/TeamShapeModel.h"

struct ShotTargetContext {
    PitchPoint shotOrigin;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    double shooterFinishing = 0.0;
    double shooterTechnique = 0.0;
    double shooterComposure = 0.0;
    double pressure = 0.0;
};

struct ShotTargetResult {
    PitchPoint intendedTarget;
    double targetDifficulty = 0.0;
    double placementQuality = 0.0;
};

class ShotTargetModel {
public:
    ShotTargetResult chooseTarget(const ShotTargetContext& context) const;
};
