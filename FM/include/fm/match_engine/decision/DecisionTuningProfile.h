#pragma once

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
