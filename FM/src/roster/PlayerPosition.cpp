#include"fm/roster/PlayerPosition.h"

#include<algorithm>
#include<cctype>
#include<stdexcept>
#include<string_view>
#include<unordered_map>

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
const std::unordered_map<std::string_view, PlayerPosition>& playerPositionLookup() {
    // Boundary parsing table:
    // - canonical tokens are the official short DB/input syntax
    // - compatibility aliases are accepted legacy variants
    // - legacy broad roles are compatibility-only fallbacks
    static const std::unordered_map<std::string_view, PlayerPosition> kLookup = {
        // Canonical tokens (official boundary syntax)
        {"GK", PlayerPosition::Goalkeeper},
        {"CB", PlayerPosition::CenterBack},
        {"LB", PlayerPosition::LeftBack},
        {"RB", PlayerPosition::RightBack},
        {"DM", PlayerPosition::DefensiveMidfielder},
        {"CM", PlayerPosition::CentralMidfielder},
        {"AM", PlayerPosition::AttackingMidfielder},
        {"LM", PlayerPosition::LeftMidfielder},
        {"RM", PlayerPosition::RightMidfielder},
        {"LW", PlayerPosition::LeftWinger},
        {"RW", PlayerPosition::RightWinger},
        {"ST", PlayerPosition::Striker},

        // Compatibility aliases
        {"GOALKEEPER", PlayerPosition::Goalkeeper},
        {"CENTERBACK", PlayerPosition::CenterBack},
        {"CENTREBACK", PlayerPosition::CenterBack},
        {"LEFTBACK", PlayerPosition::LeftBack},
        {"RIGHTBACK", PlayerPosition::RightBack},
        {"CDM", PlayerPosition::DefensiveMidfielder},
        {"DEFENSIVEMIDFIELDER", PlayerPosition::DefensiveMidfielder},
        {"CENTRALMIDFIELDER", PlayerPosition::CentralMidfielder},
        {"CAM", PlayerPosition::AttackingMidfielder},
        {"ATTACKINGMIDFIELDER", PlayerPosition::AttackingMidfielder},
        {"LEFTMIDFIELDER", PlayerPosition::LeftMidfielder},
        {"RIGHTMIDFIELDER", PlayerPosition::RightMidfielder},
        {"LEFTWINGER", PlayerPosition::LeftWinger},
        {"RIGHTWINGER", PlayerPosition::RightWinger},
        {"STRIKER", PlayerPosition::Striker},
        {"FW", PlayerPosition::Striker},
        {"FWD", PlayerPosition::Striker},
        {"FORWARD", PlayerPosition::Striker},

        // Legacy broad roles (compatibility-only)
        {"DEF", PlayerPosition::CenterBack},
        {"DF", PlayerPosition::CenterBack},
        {"DEFENDER", PlayerPosition::CenterBack},
        {"MID", PlayerPosition::CentralMidfielder},
        {"MF", PlayerPosition::CentralMidfielder},
        {"MIDFIELDER", PlayerPosition::CentralMidfielder}
    };

    return kLookup;
}
}

PlayerPosition parsePlayerPosition(const std::string& position) {
    const std::string normalized = normalizePosition(position);

    const auto& lookup = playerPositionLookup();
    const auto it = lookup.find(normalized);

    if (it != lookup.end()) {
        return it->second;
        throw std::invalid_argument("Unsupported player position: " + position);
    }
}

std::string toDisplayString(PlayerPosition position) {
    switch (position) {
    case PlayerPosition::Goalkeeper:
        return "Goalkeeper";
    case PlayerPosition::CenterBack:
        return "Center Back";
    case PlayerPosition::LeftBack:
        return "Left Back";
    case PlayerPosition::RightBack:
        return "Right Back";
    case PlayerPosition::DefensiveMidfielder:
        return "Defensive Midfielder";
    case PlayerPosition::CentralMidfielder:
        return "Central Midfielder";
    case PlayerPosition::AttackingMidfielder:
        return "Attacking Midfielder";
    case PlayerPosition::LeftMidfielder:
        return "Left Midfielder";
    case PlayerPosition::RightMidfielder:
        return "Right Midfielder";
    case PlayerPosition::LeftWinger:
        return "Left Winger";
    case PlayerPosition::RightWinger:
        return "Right Winger";
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
