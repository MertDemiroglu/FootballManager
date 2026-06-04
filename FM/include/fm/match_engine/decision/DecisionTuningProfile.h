#pragma once

#include"fm/match/TacticalSetup.h"
#include"fm/roster/Formation.h"

#include<algorithm>

// PR87 only creates the decision tuning home. PR88/PR89 can move pass,
// carry, and shot constants here without mixing tuning into simulation code.

struct RoleRiskProfile {
    double safeOptionBias = 1.0;
    double progressiveOptionBias = 1.0;
    double directOptionBias = 1.0;
    double wideOptionBias = 1.0;
    double crossOptionBias = 1.0;
    double throughBallBias = 1.0;
    double riskTolerance = 1.0;
};

struct TacticalBiasProfile {
    double possessionBias = 1.0;
    double verticalityBias = 1.0;
    double widthBias = 1.0;
    double safeCirculationBias = 1.0;
    double directPassingBias = 1.0;
    double riskTolerance = 1.0;
};

struct PassDecisionTuning {
    double safePassBaseline = 1.0;
    double backPassBaseline = 1.0;
    double progressivePassBaseline = 1.0;
    double switchPlayBaseline = 1.0;
    double throughBallBaseline = 1.0;
    double crossBaseline = 1.0;
    double cutbackBaseline = 1.0;
    double laneRiskPenaltyScale = 1.0;
    double receiverPressurePenaltyScale = 1.0;
};

struct CarryDecisionTuning {
    double safeCarryBaseline = 1.0;
    double progressiveCarryBaseline = 1.0;
    double dribbleBaseline = 1.0;
};

struct ShotDecisionTuning {
    double openPlayShotBaseline = 1.0;
    double pressurePenaltyScale = 1.0;
};

struct ActionScoringWeights {
    double retentionWeight = 1.0;
    double progressionWeight = 1.0;
    double chanceWeight = 1.0;
    double tacticalFitWeight = 1.0;
    double roleFitWeight = 1.0;
    double skillFitWeight = 1.0;
    double pressureCostWeight = 1.0;
    double turnoverRiskWeight = 1.0;
    double phaseFitWeight = 1.0;
};

struct RoleDecisionProfile {
    double passRiskTolerance = 1.0;
    double carryRiskTolerance = 1.0;
    double shotRiskTolerance = 1.0;
    double retentionPreference = 1.0;
    double progressionPreference = 1.0;
    double chancePreference = 1.0;
    double widePreference = 1.0;
    double directPreference = 1.0;
};

struct TacticalDecisionProfile {
    double retentionIntent = 1.0;
    double progressionIntent = 1.0;
    double chanceIntent = 1.0;
    double riskTolerance = 1.0;
    double widthIntent = 1.0;
    double directnessIntent = 1.0;
};

struct ActionSelectionTuning {
    double decisionSharpnessBase = 0.75;
    double decisionSharpnessFromDecisions = 35.0;
    double minimumCandidateWeightScore = 0.1;
    double passSkillInfluence = 160.0;
    double carrySkillInfluence = 190.0;
    double shotSkillInfluence = 180.0;
    double passSkillBaseline = 0.85;
    double carrySkillBaseline = 0.90;
    double shotSkillBaseline = 0.88;
    double passSkillMinimum = 0.78;
    double passSkillMaximum = 1.18;
    double carrySkillMinimum = 0.82;
    double carrySkillMaximum = 1.14;
    double shotSkillMinimum = 0.80;
    double shotSkillMaximum = 1.16;
};

struct CarryRoleDecisionProfile {
    double safeBias = 1.0;
    double progressiveBias = 1.0;
    double dribbleBias = 1.0;
    double wideBias = 1.0;
    double centralBias = 1.0;
    double riskTolerance = 1.0;
    double softProgressLimit = 0.70;
    double deepCarryPenalty = 0.0;
};

struct CarryTacticalDecisionProfile {
    double safeBias = 1.0;
    double progressiveBias = 1.0;
    double dribbleBias = 1.0;
    double riskTolerance = 1.0;
};

struct ShotRoleDecisionProfile {
    double shotBias = 1.0;
    double longShotBias = 1.0;
    double riskTolerance = 1.0;
};

struct ShotTacticalDecisionProfile {
    double shotBias = 1.0;
    double weakShotBias = 1.0;
};

inline ActionScoringWeights defaultActionScoringWeights() {
    return ActionScoringWeights{};
}

inline ActionSelectionTuning defaultActionSelectionTuning() {
    return ActionSelectionTuning{};
}

inline RoleDecisionProfile roleDecisionProfile(FormationSlotRole role) {
    RoleDecisionProfile profile;
    switch (role) {
    case FormationSlotRole::Goalkeeper:
        profile.retentionPreference = 1.12;
        profile.progressionPreference = 0.55;
        profile.chancePreference = 0.10;
        profile.passRiskTolerance = 0.62;
        profile.carryRiskTolerance = 0.42;
        profile.shotRiskTolerance = 0.30;
        break;
    case FormationSlotRole::CenterBack:
        profile.retentionPreference = 1.15;
        profile.progressionPreference = 0.76;
        profile.chancePreference = 0.34;
        profile.passRiskTolerance = 0.78;
        profile.carryRiskTolerance = 0.62;
        profile.shotRiskTolerance = 0.58;
        break;
    case FormationSlotRole::LeftBack:
    case FormationSlotRole::RightBack:
    case FormationSlotRole::LeftWingBack:
    case FormationSlotRole::RightWingBack:
        profile.widePreference = 1.18;
        profile.progressionPreference = 0.98;
        profile.chancePreference = 0.62;
        break;
    case FormationSlotRole::DefensiveMidfielder:
        profile.retentionPreference = 1.10;
        profile.progressionPreference = 0.92;
        profile.chancePreference = 0.58;
        break;
    case FormationSlotRole::CentralMidfielder:
    case FormationSlotRole::LeftMidfielder:
    case FormationSlotRole::RightMidfielder:
        profile.progressionPreference = 1.05;
        profile.chancePreference = 0.86;
        break;
    case FormationSlotRole::AttackingMidfielder:
        profile.retentionPreference = 0.92;
        profile.progressionPreference = 1.16;
        profile.chancePreference = 1.12;
        break;
    case FormationSlotRole::LeftWinger:
    case FormationSlotRole::RightWinger:
        profile.retentionPreference = 0.90;
        profile.progressionPreference = 1.14;
        profile.chancePreference = 1.04;
        profile.widePreference = 1.22;
        break;
    case FormationSlotRole::Striker:
        profile.retentionPreference = 0.82;
        profile.progressionPreference = 0.92;
        profile.chancePreference = 1.22;
        break;
    case FormationSlotRole::Unknown:
        break;
    }
    return profile;
}

inline TacticalDecisionProfile tacticalDecisionProfile(const TacticalSetup& tactics) {
    TacticalDecisionProfile profile;
    if (tactics.mentality == TeamMentality::Defensive) {
        profile.retentionIntent += 0.14;
        profile.progressionIntent -= 0.10;
        profile.chanceIntent -= 0.12;
        profile.riskTolerance -= 0.10;
    } else if (tactics.mentality == TeamMentality::Attacking) {
        profile.retentionIntent -= 0.06;
        profile.progressionIntent += 0.12;
        profile.chanceIntent += 0.12;
        profile.riskTolerance += 0.10;
    }

    if (tactics.tempo == TeamTempo::Low) {
        profile.retentionIntent += 0.12;
        profile.progressionIntent -= 0.06;
        profile.chanceIntent -= 0.04;
    } else if (tactics.tempo == TeamTempo::High) {
        profile.retentionIntent -= 0.04;
        profile.progressionIntent += 0.10;
        profile.chanceIntent += 0.06;
    }

    if (tactics.passingDirectness == PassingDirectness::Short) {
        profile.retentionIntent += 0.10;
        profile.directnessIntent -= 0.12;
    } else if (tactics.passingDirectness == PassingDirectness::Direct) {
        profile.retentionIntent -= 0.05;
        profile.progressionIntent += 0.12;
        profile.directnessIntent += 0.16;
    }

    profile.retentionIntent = std::clamp(profile.retentionIntent, 0.70, 1.35);
    profile.progressionIntent = std::clamp(profile.progressionIntent, 0.70, 1.35);
    profile.chanceIntent = std::clamp(profile.chanceIntent, 0.70, 1.35);
    profile.riskTolerance = std::clamp(profile.riskTolerance, 0.65, 1.35);
    profile.directnessIntent = std::clamp(profile.directnessIntent, 0.70, 1.35);
    return profile;
}

inline RoleRiskProfile passRoleRiskProfile(FormationSlotRole role) {
    RoleRiskProfile profile;
    switch (role) {
    case FormationSlotRole::Goalkeeper:
        profile.safeOptionBias = 1.45;
        profile.progressiveOptionBias = 0.45;
        profile.directOptionBias = 0.62;
        profile.wideOptionBias = 0.70;
        profile.crossOptionBias = 0.05;
        profile.throughBallBias = 0.05;
        profile.riskTolerance = 0.58;
        break;
    case FormationSlotRole::CenterBack:
        profile.safeOptionBias = 1.34;
        profile.progressiveOptionBias = 0.68;
        profile.directOptionBias = 0.70;
        profile.wideOptionBias = 0.86;
        profile.crossOptionBias = 0.10;
        profile.throughBallBias = 0.20;
        profile.riskTolerance = 0.66;
        break;
    case FormationSlotRole::LeftBack:
    case FormationSlotRole::RightBack:
    case FormationSlotRole::LeftWingBack:
    case FormationSlotRole::RightWingBack:
        profile.safeOptionBias = 1.14;
        profile.progressiveOptionBias = 1.00;
        profile.directOptionBias = 0.92;
        profile.wideOptionBias = 1.25;
        profile.crossOptionBias = 1.18;
        profile.throughBallBias = 0.55;
        profile.riskTolerance = 0.84;
        break;
    case FormationSlotRole::DefensiveMidfielder:
        profile.safeOptionBias = 1.22;
        profile.progressiveOptionBias = 0.96;
        profile.directOptionBias = 0.86;
        profile.wideOptionBias = 0.96;
        profile.crossOptionBias = 0.30;
        profile.throughBallBias = 0.60;
        profile.riskTolerance = 0.82;
        break;
    case FormationSlotRole::CentralMidfielder:
        profile.safeOptionBias = 1.05;
        profile.progressiveOptionBias = 1.10;
        profile.directOptionBias = 1.00;
        profile.wideOptionBias = 1.08;
        profile.crossOptionBias = 0.55;
        profile.throughBallBias = 0.85;
        profile.riskTolerance = 0.98;
        break;
    case FormationSlotRole::LeftMidfielder:
    case FormationSlotRole::RightMidfielder:
        profile.safeOptionBias = 1.05;
        profile.progressiveOptionBias = 1.10;
        profile.directOptionBias = 1.00;
        profile.wideOptionBias = 1.08;
        profile.crossOptionBias = 1.05;
        profile.throughBallBias = 0.85;
        profile.riskTolerance = 0.98;
        break;
    case FormationSlotRole::AttackingMidfielder:
        profile.safeOptionBias = 0.88;
        profile.progressiveOptionBias = 1.25;
        profile.directOptionBias = 1.18;
        profile.wideOptionBias = 0.95;
        profile.crossOptionBias = 0.82;
        profile.throughBallBias = 1.24;
        profile.riskTolerance = 1.18;
        break;
    case FormationSlotRole::LeftWinger:
    case FormationSlotRole::RightWinger:
        profile.safeOptionBias = 0.88;
        profile.progressiveOptionBias = 1.12;
        profile.directOptionBias = 1.00;
        profile.wideOptionBias = 1.24;
        profile.crossOptionBias = 1.32;
        profile.throughBallBias = 0.76;
        profile.riskTolerance = 1.08;
        break;
    case FormationSlotRole::Striker:
        profile.safeOptionBias = 0.96;
        profile.progressiveOptionBias = 0.86;
        profile.directOptionBias = 0.80;
        profile.wideOptionBias = 0.72;
        profile.crossOptionBias = 0.45;
        profile.throughBallBias = 0.55;
        profile.riskTolerance = 0.92;
        break;
    case FormationSlotRole::Unknown:
        break;
    }

    return profile;
}

inline TacticalBiasProfile passTacticalBiasProfile(const TacticalSetup& tactics) {
    TacticalBiasProfile profile;
    if (tactics.mentality == TeamMentality::Defensive) {
        profile.safeCirculationBias += 0.22;
        profile.possessionBias += 0.08;
        profile.verticalityBias -= 0.14;
        profile.directPassingBias -= 0.08;
        profile.riskTolerance -= 0.14;
    } else if (tactics.mentality == TeamMentality::Attacking) {
        profile.safeCirculationBias -= 0.08;
        profile.verticalityBias += 0.20;
        profile.directPassingBias += 0.14;
        profile.riskTolerance += 0.16;
    }

    if (tactics.tempo == TeamTempo::Low) {
        profile.safeCirculationBias += 0.16;
        profile.possessionBias += 0.12;
        profile.verticalityBias -= 0.10;
        profile.riskTolerance -= 0.06;
    } else if (tactics.tempo == TeamTempo::High) {
        profile.safeCirculationBias -= 0.08;
        profile.verticalityBias += 0.16;
        profile.directPassingBias += 0.12;
        profile.riskTolerance += 0.10;
    }

    if (tactics.passingDirectness == PassingDirectness::Short) {
        profile.safeCirculationBias += 0.22;
        profile.possessionBias += 0.15;
        profile.verticalityBias -= 0.12;
        profile.directPassingBias -= 0.18;
    } else if (tactics.passingDirectness == PassingDirectness::Direct) {
        profile.safeCirculationBias -= 0.14;
        profile.verticalityBias += 0.24;
        profile.directPassingBias += 0.24;
        profile.riskTolerance += 0.12;
    }

    if (tactics.width == TeamWidth::Wide) {
        profile.widthBias += 0.22;
    } else if (tactics.width == TeamWidth::Narrow) {
        profile.widthBias -= 0.16;
        profile.possessionBias += 0.05;
    }

    profile.safeCirculationBias = std::clamp(profile.safeCirculationBias, 0.65, 1.45);
    profile.possessionBias = std::clamp(profile.possessionBias, 0.70, 1.35);
    profile.verticalityBias = std::clamp(profile.verticalityBias, 0.65, 1.45);
    profile.directPassingBias = std::clamp(profile.directPassingBias, 0.65, 1.45);
    profile.widthBias = std::clamp(profile.widthBias, 0.65, 1.45);
    profile.riskTolerance = std::clamp(profile.riskTolerance, 0.70, 1.35);
    return profile;
}

inline CarryRoleDecisionProfile carryRoleDecisionProfile(FormationSlotRole role) {
    CarryRoleDecisionProfile profile;
    switch (role) {
    case FormationSlotRole::Goalkeeper:
        profile.safeBias = 0.30;
        profile.progressiveBias = 0.05;
        profile.dribbleBias = 0.02;
        profile.riskTolerance = 0.42;
        profile.softProgressLimit = 0.22;
        break;
    case FormationSlotRole::CenterBack:
        profile.safeBias = 1.18;
        profile.progressiveBias = 0.55;
        profile.dribbleBias = 0.10;
        profile.riskTolerance = 0.62;
        profile.softProgressLimit = 0.46;
        break;
    case FormationSlotRole::LeftBack:
    case FormationSlotRole::RightBack:
        profile.safeBias = 1.08;
        profile.progressiveBias = 0.92;
        profile.dribbleBias = 0.58;
        profile.wideBias = 1.25;
        profile.riskTolerance = 0.82;
        profile.softProgressLimit = 0.66;
        break;
    case FormationSlotRole::LeftWingBack:
    case FormationSlotRole::RightWingBack:
        profile.safeBias = 1.02;
        profile.progressiveBias = 1.08;
        profile.dribbleBias = 0.76;
        profile.wideBias = 1.30;
        profile.riskTolerance = 0.92;
        profile.softProgressLimit = 0.74;
        break;
    case FormationSlotRole::DefensiveMidfielder:
        profile.safeBias = 1.14;
        profile.progressiveBias = 0.88;
        profile.dribbleBias = 0.38;
        profile.riskTolerance = 0.78;
        profile.softProgressLimit = 0.58;
        break;
    case FormationSlotRole::CentralMidfielder:
        profile.safeBias = 1.02;
        profile.progressiveBias = 1.04;
        profile.dribbleBias = 0.72;
        profile.riskTolerance = 0.96;
        profile.softProgressLimit = 0.70;
        break;
    case FormationSlotRole::LeftMidfielder:
    case FormationSlotRole::RightMidfielder:
        profile.safeBias = 0.98;
        profile.progressiveBias = 1.08;
        profile.dribbleBias = 0.86;
        profile.wideBias = 1.16;
        profile.riskTolerance = 1.00;
        profile.softProgressLimit = 0.76;
        break;
    case FormationSlotRole::AttackingMidfielder:
        profile.safeBias = 0.86;
        profile.progressiveBias = 1.22;
        profile.dribbleBias = 1.16;
        profile.centralBias = 1.12;
        profile.riskTolerance = 1.12;
        profile.softProgressLimit = 0.88;
        break;
    case FormationSlotRole::LeftWinger:
    case FormationSlotRole::RightWinger:
        profile.safeBias = 0.84;
        profile.progressiveBias = 1.24;
        profile.dribbleBias = 1.28;
        profile.wideBias = 1.34;
        profile.riskTolerance = 1.10;
        profile.softProgressLimit = 0.92;
        break;
    case FormationSlotRole::Striker:
        profile.safeBias = 0.70;
        profile.progressiveBias = 0.82;
        profile.dribbleBias = 1.02;
        profile.riskTolerance = 0.94;
        profile.softProgressLimit = 0.94;
        profile.deepCarryPenalty = 14.0;
        break;
    case FormationSlotRole::Unknown:
        break;
    }
    return profile;
}

inline CarryTacticalDecisionProfile carryTacticalDecisionProfile(const TacticalSetup& tactics) {
    CarryTacticalDecisionProfile profile;
    if (tactics.mentality == TeamMentality::Defensive) {
        profile.safeBias += 0.18;
        profile.progressiveBias -= 0.14;
        profile.dribbleBias -= 0.16;
        profile.riskTolerance -= 0.12;
    } else if (tactics.mentality == TeamMentality::Attacking) {
        profile.safeBias -= 0.06;
        profile.progressiveBias += 0.18;
        profile.dribbleBias += 0.14;
        profile.riskTolerance += 0.14;
    }

    if (tactics.tempo == TeamTempo::Low) {
        profile.safeBias += 0.16;
        profile.progressiveBias -= 0.10;
        profile.dribbleBias -= 0.08;
        profile.riskTolerance -= 0.05;
    } else if (tactics.tempo == TeamTempo::High) {
        profile.safeBias -= 0.06;
        profile.progressiveBias += 0.16;
        profile.dribbleBias += 0.11;
        profile.riskTolerance += 0.08;
    }

    if (tactics.passingDirectness == PassingDirectness::Short) {
        profile.safeBias += 0.12;
        profile.progressiveBias -= 0.10;
    } else if (tactics.passingDirectness == PassingDirectness::Direct) {
        profile.safeBias -= 0.08;
        profile.progressiveBias += 0.16;
        profile.dribbleBias += 0.06;
    }

    profile.safeBias = std::clamp(profile.safeBias, 0.65, 1.40);
    profile.progressiveBias = std::clamp(profile.progressiveBias, 0.55, 1.45);
    profile.dribbleBias = std::clamp(profile.dribbleBias, 0.50, 1.45);
    profile.riskTolerance = std::clamp(profile.riskTolerance, 0.65, 1.35);
    return profile;
}

inline ShotRoleDecisionProfile shotRoleDecisionProfile(FormationSlotRole role) {
    ShotRoleDecisionProfile profile;
    switch (role) {
    case FormationSlotRole::Goalkeeper:
        profile.shotBias = 0.12;
        profile.longShotBias = 0.08;
        profile.riskTolerance = 0.45;
        break;
    case FormationSlotRole::CenterBack:
        profile.shotBias = 0.34;
        profile.longShotBias = 0.22;
        profile.riskTolerance = 0.58;
        break;
    case FormationSlotRole::LeftBack:
    case FormationSlotRole::RightBack:
    case FormationSlotRole::LeftWingBack:
    case FormationSlotRole::RightWingBack:
        profile.shotBias = 0.58;
        profile.longShotBias = 0.46;
        profile.riskTolerance = 0.72;
        break;
    case FormationSlotRole::DefensiveMidfielder:
        profile.shotBias = 0.66;
        profile.longShotBias = 0.56;
        profile.riskTolerance = 0.74;
        break;
    case FormationSlotRole::CentralMidfielder:
    case FormationSlotRole::LeftMidfielder:
    case FormationSlotRole::RightMidfielder:
        profile.shotBias = 0.88;
        profile.longShotBias = 0.88;
        profile.riskTolerance = 0.92;
        break;
    case FormationSlotRole::AttackingMidfielder:
        profile.shotBias = 1.08;
        profile.longShotBias = 1.02;
        profile.riskTolerance = 1.10;
        break;
    case FormationSlotRole::LeftWinger:
    case FormationSlotRole::RightWinger:
        profile.shotBias = 0.98;
        profile.longShotBias = 0.92;
        profile.riskTolerance = 1.03;
        break;
    case FormationSlotRole::Striker:
        profile.shotBias = 1.18;
        profile.longShotBias = 1.02;
        profile.riskTolerance = 1.12;
        break;
    case FormationSlotRole::Unknown:
        break;
    }
    return profile;
}

inline ShotTacticalDecisionProfile shotTacticalDecisionProfile(const TacticalSetup& tactics) {
    ShotTacticalDecisionProfile profile;
    if (tactics.mentality == TeamMentality::Defensive) {
        profile.shotBias -= 0.16;
        profile.weakShotBias -= 0.22;
    } else if (tactics.mentality == TeamMentality::Attacking) {
        profile.shotBias += 0.14;
        profile.weakShotBias += 0.08;
    }

    if (tactics.tempo == TeamTempo::Low) {
        profile.shotBias -= 0.06;
        profile.weakShotBias -= 0.12;
    } else if (tactics.tempo == TeamTempo::High) {
        profile.shotBias += 0.07;
        profile.weakShotBias += 0.05;
    }

    if (tactics.passingDirectness == PassingDirectness::Direct) {
        profile.shotBias += 0.05;
    } else if (tactics.passingDirectness == PassingDirectness::Short) {
        profile.weakShotBias -= 0.08;
    }

    profile.shotBias = std::clamp(profile.shotBias, 0.55, 1.35);
    profile.weakShotBias = std::clamp(profile.weakShotBias, 0.50, 1.25);
    return profile;
}
