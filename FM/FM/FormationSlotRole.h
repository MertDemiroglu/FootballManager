#pragma once

enum class FormationSlotRole {
    Unknown,
    Goalkeeper,
    CenterBack,
    LeftBack,
    RightBack,
    LeftWingBack,
    RightWingBack,
    DefensiveMidfielder,
    CentralMidfielder,
    AttackingMidfielder,
    LeftMidfielder,
    RightMidfielder,
    LeftWinger,
    RightWinger,
    Striker
};

bool isSlotRoleSupported(FormationSlotRole role);
