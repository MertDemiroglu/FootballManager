#pragma once

#include"fm/match_engine/PitchGeometry.h"
#include"fm/match_engine/TeamShapeModel.h"

enum class TacticalZone {
    Unknown,

    DefensiveLeft,
    DefensiveCenter,
    DefensiveRight,

    MiddleLeft,
    MiddleCenter,
    MiddleRight,

    AttackingLeft,
    AttackingCenter,
    AttackingRight
};

TacticalZone tacticalZoneForPoint(PitchPoint point, AttackingDirection direction);

bool isWideZone(TacticalZone zone);
bool isCentralZone(TacticalZone zone);
bool isDefensiveThird(TacticalZone zone);
bool isMiddleThird(TacticalZone zone);
bool isAttackingThird(TacticalZone zone);

bool sameVerticalLane(TacticalZone a, TacticalZone b);
bool adjacentLane(TacticalZone a, TacticalZone b);
bool sameOrAdjacentLane(TacticalZone a, TacticalZone b);
