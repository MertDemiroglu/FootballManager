#pragma once

#include<optional>
#include<string_view>

enum class TeamMentality {
    Defensive,
    Balanced,
    Attacking
};

enum class TeamTempo {
    Low,
    Normal,
    High
};

struct TacticalSetup {
    TeamMentality mentality = TeamMentality::Balanced;
    TeamTempo tempo = TeamTempo::Normal;
};

inline std::string_view toStableCode(TeamMentality mentality) {
    switch (mentality) {
    case TeamMentality::Defensive:
        return "defensive";
    case TeamMentality::Balanced:
        return "balanced";
    case TeamMentality::Attacking:
        return "attacking";
    }

    return "balanced";
}

inline std::optional<TeamMentality> teamMentalityFromStableCode(std::string_view stableCode) {
    if (stableCode == "defensive") {
        return TeamMentality::Defensive;
    }
    if (stableCode == "balanced") {
        return TeamMentality::Balanced;
    }
    if (stableCode == "attacking") {
        return TeamMentality::Attacking;
    }

    return std::nullopt;
}

inline std::string_view toDisplayText(TeamMentality mentality) {
    switch (mentality) {
    case TeamMentality::Defensive:
        return "Defensive";
    case TeamMentality::Balanced:
        return "Balanced";
    case TeamMentality::Attacking:
        return "Attacking";
    }

    return "Balanced";
}

inline std::string_view toStableCode(TeamTempo tempo) {
    switch (tempo) {
    case TeamTempo::Low:
        return "low";
    case TeamTempo::Normal:
        return "normal";
    case TeamTempo::High:
        return "high";
    }

    return "normal";
}

inline std::optional<TeamTempo> teamTempoFromStableCode(std::string_view stableCode) {
    if (stableCode == "low") {
        return TeamTempo::Low;
    }
    if (stableCode == "normal") {
        return TeamTempo::Normal;
    }
    if (stableCode == "high") {
        return TeamTempo::High;
    }

    return std::nullopt;
}

inline std::string_view toDisplayText(TeamTempo tempo) {
    switch (tempo) {
    case TeamTempo::Low:
        return "Low";
    case TeamTempo::Normal:
        return "Normal";
    case TeamTempo::High:
        return "High";
    }

    return "Normal";
}
