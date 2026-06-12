#pragma once

#include"fm/match_engine/ball/ShotTypes.h"

class ShotTargetSelector {
public:
    ShotTargetSelectionResult select(
        const ShotContext& context,
        ShotType shotType) const;
};

PitchPoint shotTargetPointFor(
    PitchPoint shotOrigin,
    AttackingDirection attackingDirection,
    ShotTargetZone zone);

ShotTargetPoint shotTargetFramePointFor(
    PitchPoint shotOrigin,
    ShotTargetZone zone);
