#pragma once

#include <string>

enum class PlayerPosition {
    Unknown,
    Goalkeeper,
    CenterBack,
    LeftBack,
    RightBack,
    DefensiveMidfielder,
    CentralMidfielder,
    AttackingMidfielder,
    LeftMidfielder,
    RightMidfielder,
    LeftWinger,
    RightWinger,
    Striker
};

PlayerPosition parsePlayerPosition(const std::string& position);
std::string toDisplayString(PlayerPosition position);
bool isKnownPlayerPosition(PlayerPosition position);
