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

enum class TeamWidth {
    Narrow,
    Balanced,
    Wide
};

enum class DefensiveLine {
    Low,
    Standard,
    High
};

enum class PressingIntensity {
    Low,
    Normal,
    High
};

enum class MarkingStyle {
    Zonal,
    Mixed,
    ManOriented
};

enum class PassingDirectness {
    Short,
    Balanced,
    Direct
};

struct TacticalSetup {
    TeamMentality mentality = TeamMentality::Balanced;
    TeamTempo tempo = TeamTempo::Normal;
    TeamWidth width = TeamWidth::Balanced;
    DefensiveLine defensiveLine = DefensiveLine::Standard;
    PressingIntensity pressingIntensity = PressingIntensity::Normal;
    MarkingStyle markingStyle = MarkingStyle::Mixed;
    PassingDirectness passingDirectness = PassingDirectness::Balanced;
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

inline std::string_view toStableCode(TeamWidth width) {
    switch (width) {
    case TeamWidth::Narrow:
        return "narrow";
    case TeamWidth::Balanced:
        return "balanced";
    case TeamWidth::Wide:
        return "wide";
    }

    return "balanced";
}

inline std::optional<TeamWidth> teamWidthFromStableCode(std::string_view stableCode) {
    if (stableCode == "narrow") {
        return TeamWidth::Narrow;
    }
    if (stableCode == "balanced") {
        return TeamWidth::Balanced;
    }
    if (stableCode == "wide") {
        return TeamWidth::Wide;
    }

    return std::nullopt;
}

inline std::string_view toDisplayText(TeamWidth width) {
    switch (width) {
    case TeamWidth::Narrow:
        return "Narrow";
    case TeamWidth::Balanced:
        return "Balanced";
    case TeamWidth::Wide:
        return "Wide";
    }

    return "Balanced";
}

inline std::string_view toStableCode(DefensiveLine defensiveLine) {
    switch (defensiveLine) {
    case DefensiveLine::Low:
        return "low";
    case DefensiveLine::Standard:
        return "standard";
    case DefensiveLine::High:
        return "high";
    }

    return "standard";
}

inline std::optional<DefensiveLine> defensiveLineFromStableCode(std::string_view stableCode) {
    if (stableCode == "low") {
        return DefensiveLine::Low;
    }
    if (stableCode == "standard") {
        return DefensiveLine::Standard;
    }
    if (stableCode == "high") {
        return DefensiveLine::High;
    }

    return std::nullopt;
}

inline std::string_view toDisplayText(DefensiveLine defensiveLine) {
    switch (defensiveLine) {
    case DefensiveLine::Low:
        return "Low";
    case DefensiveLine::Standard:
        return "Standard";
    case DefensiveLine::High:
        return "High";
    }

    return "Standard";
}

inline std::string_view toStableCode(PressingIntensity pressingIntensity) {
    switch (pressingIntensity) {
    case PressingIntensity::Low:
        return "low";
    case PressingIntensity::Normal:
        return "normal";
    case PressingIntensity::High:
        return "high";
    }

    return "normal";
}

inline std::optional<PressingIntensity> pressingIntensityFromStableCode(std::string_view stableCode) {
    if (stableCode == "low") {
        return PressingIntensity::Low;
    }
    if (stableCode == "normal") {
        return PressingIntensity::Normal;
    }
    if (stableCode == "high") {
        return PressingIntensity::High;
    }

    return std::nullopt;
}

inline std::string_view toDisplayText(PressingIntensity pressingIntensity) {
    switch (pressingIntensity) {
    case PressingIntensity::Low:
        return "Low";
    case PressingIntensity::Normal:
        return "Normal";
    case PressingIntensity::High:
        return "High";
    }

    return "Normal";
}

inline std::string_view toStableCode(MarkingStyle markingStyle) {
    switch (markingStyle) {
    case MarkingStyle::Zonal:
        return "zonal";
    case MarkingStyle::Mixed:
        return "mixed";
    case MarkingStyle::ManOriented:
        return "man_oriented";
    }

    return "mixed";
}

inline std::optional<MarkingStyle> markingStyleFromStableCode(std::string_view stableCode) {
    if (stableCode == "zonal") {
        return MarkingStyle::Zonal;
    }
    if (stableCode == "mixed") {
        return MarkingStyle::Mixed;
    }
    if (stableCode == "man_oriented") {
        return MarkingStyle::ManOriented;
    }

    return std::nullopt;
}

inline std::string_view toDisplayText(MarkingStyle markingStyle) {
    switch (markingStyle) {
    case MarkingStyle::Zonal:
        return "Zonal";
    case MarkingStyle::Mixed:
        return "Mixed";
    case MarkingStyle::ManOriented:
        return "Man Oriented";
    }

    return "Mixed";
}

inline std::string_view toStableCode(PassingDirectness passingDirectness) {
    switch (passingDirectness) {
    case PassingDirectness::Short:
        return "short";
    case PassingDirectness::Balanced:
        return "balanced";
    case PassingDirectness::Direct:
        return "direct";
    }

    return "balanced";
}

inline std::optional<PassingDirectness> passingDirectnessFromStableCode(std::string_view stableCode) {
    if (stableCode == "short") {
        return PassingDirectness::Short;
    }
    if (stableCode == "balanced") {
        return PassingDirectness::Balanced;
    }
    if (stableCode == "direct") {
        return PassingDirectness::Direct;
    }

    return std::nullopt;
}

inline std::string_view toDisplayText(PassingDirectness passingDirectness) {
    switch (passingDirectness) {
    case PassingDirectness::Short:
        return "Short";
    case PassingDirectness::Balanced:
        return "Balanced";
    case PassingDirectness::Direct:
        return "Direct";
    }

    return "Balanced";
}
