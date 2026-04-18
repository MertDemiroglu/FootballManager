#include"fm/roster/PlayerPosition.h"

#include<algorithm>
#include<cctype>
#include<stdexcept>

namespace {
std::string normalizePosition(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (const unsigned char c : value) {
        if (!std::isspace(c) && c != '-' && c != '_') {
            normalized.push_back(static_cast<char>(std::toupper(c)));
        }
    }

    return normalized;
}
}

PlayerPosition parsePlayerPosition(const std::string& position) {
    const std::string normalized = normalizePosition(position);

    if (normalized == "GK" || normalized == "GOALKEEPER") {
        return PlayerPosition::Goalkeeper;
    }

    if (normalized == "CB" || normalized == "CENTERBACK" || normalized == "CENTREBACK") {
        return PlayerPosition::CenterBack;
    }

    if (normalized == "LB" || normalized == "LEFTBACK") {
        return PlayerPosition::LeftBack;
    }

    if (normalized == "RB" || normalized == "RIGHTBACK") {
        return PlayerPosition::RightBack;
    }

    if (normalized == "DM" || normalized == "CDM" || normalized == "DEFENSIVEMIDFIELDER") {
        return PlayerPosition::DefensiveMidfielder;
    }

    if (normalized == "CM" || normalized == "CENTRALMIDFIELDER") {
        return PlayerPosition::CentralMidfielder;
    }

    if (normalized == "AM" || normalized == "CAM" || normalized == "ATTACKINGMIDFIELDER") {
        return PlayerPosition::AttackingMidfielder;
    }

    if (normalized == "LM" || normalized == "LEFTMIDFIELDER") {
        return PlayerPosition::LeftMidfielder;
    }

    if (normalized == "RM" || normalized == "RIGHTMIDFIELDER") {
        return PlayerPosition::RightMidfielder;
    }

    if (normalized == "LW" || normalized == "LEFTWINGER") {
        return PlayerPosition::LeftWinger;
    }

    if (normalized == "RW" || normalized == "RIGHTWINGER") {
        return PlayerPosition::RightWinger;
    }

    if (normalized == "ST" || normalized == "STRIKER" || normalized == "FW" || normalized == "FWD" || normalized == "FORWARD") {
        return PlayerPosition::Striker;
    }

    // Legacy broad roles are parsed at the boundary for compatibility.
    if (normalized == "DEF" || normalized == "DF" || normalized == "DEFENDER") {
        return PlayerPosition::CenterBack;
    }

    if (normalized == "MID" || normalized == "MF" || normalized == "MIDFIELDER") {
        return PlayerPosition::CentralMidfielder;
    }

    throw std::invalid_argument("Unsupported player position: " + position);
}

std::string toDisplayString(PlayerPosition position) {
    switch (position) {
    case PlayerPosition::Goalkeeper:
        return "Goalkeeper";
    case PlayerPosition::CenterBack:
        return "CenterBack";
    case PlayerPosition::LeftBack:
        return "LeftBack";
    case PlayerPosition::RightBack:
        return "RightBack";
    case PlayerPosition::DefensiveMidfielder:
        return "DefensiveMidfielder";
    case PlayerPosition::CentralMidfielder:
        return "CentralMidfielder";
    case PlayerPosition::AttackingMidfielder:
        return "AttackingMidfielder";
    case PlayerPosition::LeftMidfielder:
        return "LeftMidfielder";
    case PlayerPosition::RightMidfielder:
        return "RightMidfielder";
    case PlayerPosition::LeftWinger:
        return "LeftWinger";
    case PlayerPosition::RightWinger:
        return "RightWinger";
    case PlayerPosition::Striker:
        return "Striker";
    case PlayerPosition::Unknown:
        break;
    }

    return "Unknown";
}

bool isKnownPlayerPosition(PlayerPosition position) {
    return position != PlayerPosition::Unknown;
}
