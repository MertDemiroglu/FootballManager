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
    LeftWinger,
    RightWinger,
    Striker
};

bool isSlotRoleSupported(FormationSlotRole role);
