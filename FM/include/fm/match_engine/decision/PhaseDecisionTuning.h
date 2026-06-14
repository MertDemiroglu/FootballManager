#pragma once

struct PhaseDecisionTuning {
    double progressivePassMeters = 6.0;
    double progressiveCarryMeters = 4.0;
    double recycleBackwardMeters = -1.0;
    double advancedWideFinalActionProgress = 0.68;
    double exceptionalShotScore = 58.0;
    double counterForwardLaneThreshold = 0.58;

    double buildUpSafePassMultiplier = 1.16;
    double buildUpProgressivePassMultiplier = 1.08;
    double buildUpRecycleMultiplier = 1.12;
    double buildUpFoundationTargetMultiplier = 1.15;
    double buildUpStrikerTargetMultiplier = 0.70;
    double buildUpFinalBallMultiplier = 0.46;
    double buildUpAdvancedFinalBallMultiplier = 0.76;
    double buildUpShotMultiplier = 0.34;
    double buildUpExceptionalShotMultiplier = 0.72;
    double buildUpCarryMultiplier = 1.06;
    double buildUpRiskCarryMultiplier = 0.82;

    double finalizingFinalBallMultiplier = 1.05;
    double finalizingShotMultiplier = 2.50;
    double finalizingSupportRoleShotMultiplier = 3.20;
    double finalizingSupportTargetMultiplier = 1.00;
    double finalizingRecycleMultiplier = 0.64;

    double counterForwardPassMultiplier = 1.24;
    double counterForwardCarryMultiplier = 1.18;
    double counterThroughBallMultiplier = 1.16;
    double counterCreateActionMultiplier = 1.06;
    double counterRecycleWithLaneMultiplier = 0.76;
    double counterRecycleNoLaneMultiplier = 1.06;

    double defensivePhaseActionMultiplier = 0.05;
    double minimumSelectionScore = 0.1;
    double maximumSelectionScore = 100.0;
};

inline PhaseDecisionTuning defaultPhaseDecisionTuning() {
    return PhaseDecisionTuning{};
}
