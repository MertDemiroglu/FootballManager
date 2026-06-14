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
    double safePassBaseScore = 33.0;
    double backPassBaseScore = 32.0;
    double progressivePassBaseScore = 28.0;
    double switchPlayBaseScore = 25.0;
    double throughBallBaseScore = 22.0;
    double crossBaseScore = 24.0;
    double cutbackBaseScore = 24.0;
    double fallbackBaseScore = 24.0;
    double baseSafetyValue = 95.0;
    double laneRiskPenaltyScale = 1.0;
    double receiverPressurePenaltyScale = 1.0;
    double laneRiskToSafetyPenalty = 0.78;
    double receiverPressureToSafetyPenalty = 0.48;
    double difficultyToSafetyPenalty = 0.50;
    double carrierPressureToSafetyPenalty = 0.18;
    double riskToleranceMinimum = 0.55;
    double riskToleranceMaximum = 1.55;
    double safePassSafetyContribution = 0.34;
    double riskyPassSafetyContribution = 0.20;
    double safePassProgressionContribution = 0.10;
    double riskyPassProgressionContribution = 0.33;
    double passingSkillContribution = 0.10;
    double difficultyPenalty = 0.12;
    double shortSafePassDistance = 15.0;
    double shortSafePassBonus = 5.0;
    double finalThirdSimplePassBonus = 9.0;
    double defenderBackPassBonus = 3.0;
    double roleMultiplierBlend = 0.62;
    double tacticalMultiplierBlend = 0.62;
    double roleMultiplierMinimum = 0.58;
    double roleMultiplierMaximum = 1.34;
    double tacticalMultiplierMinimum = 0.58;
    double tacticalMultiplierMaximum = 1.34;
    double progressivePassPenalty = 6.0;
    double switchPlayPenalty = 8.0;
    double throughBallPenalty = 18.0;
    double crossOrCutbackPenalty = 20.0;
    double progressionForwardCap = 38.0;
    double progressionForwardMultiplier = 1.35;
    double finalThirdProgressionBonus = 8.0;
    double midfieldReceiverSafeProgressionBonus = 3.0;
    double attackingReceiverThroughBallBonus = 8.0;
    double attackingReceiverProgressionBonus = 5.0;
    double wideSwitchReceiverBonus = 6.0;
    double switchPlayProgressionBonus = 7.0;
    double throughBallProgressionBonus = 13.0;
    double crossOrCutbackProgressionBonus = 10.0;
    double offsideRiskScorePenalty = 0.62;
    double backPassProgressionMultiplier = 0.18;
    double centralSimpleReceptionMinimumGoalDistance = 11.0;
    double centralThroughBallMinimumGoalDistance = 8.5;
    double centralCrossOrCutbackMinimumGoalDistance = 7.5;
    double halfSpaceReceptionMinimumGoalDistance = 9.5;
    double halfSpaceFinalBallReceptionMinimumGoalDistance = 7.0;
    double minimumReceptionGoalLineDistance = 7.0;
};

struct CarryDecisionTuning {
    double safeCarryBaseScore = 35.0;
    double progressiveCarryBaseScore = 34.0;
    double dribbleBaseScore = 27.0;
    double fallbackBaseScore = 20.0;
    double safeSpaceContribution = 0.27;
    double riskSpaceContribution = 0.20;
    double safeProgressionContribution = 0.12;
    double riskProgressionContribution = 0.34;
    double dribbleSkillContribution = 0.25;
    double carrySkillContribution = 0.13;
    double difficultyPenalty = 0.15;
    double pressureRiskPenalty = 0.18;
    double zoneLimitRiskPenalty = 0.34;
    double riskToleranceMinimum = 0.45;
    double riskToleranceMaximum = 1.55;
    double lowPressureSafeCarryThreshold = 8.0;
    double lowPressureSafeCarryBonus = 5.0;
    double openSpaceProgressiveThreshold = 72.0;
    double openSpaceProgressiveBonus = 4.0;
    double highPressureDribbleThreshold = 35.0;
    double highPressureDribblePenalty = 8.0;
    double wideSafeCarryBonus = 2.0;
    double wideProgressiveCarryBonus = 6.0;
    double goalkeeperMinimumSafeCarryScore = 14.0;
    double minimumSafeCarryScore = 18.0;
    double minimumProgressiveCarryScore = 12.0;
    double minimumDribbleScore = 8.0;
    double minimumUsefulCarryDistance = 1.25;
    double centralSafeCarryMinimumGoalDistance = 13.0;
    double centralProgressiveCarryMinimumGoalDistance = 9.5;
    double centralDribbleMinimumGoalDistance = 8.5;
    double halfSpaceCarryMinimumGoalDistance = 8.0;
    double minimumCarryGoalLineDistance = 7.0;
    double goalmouthPressureRiskDistance = 12.0;
    double sixYardPressureRisk = 44.0;
    double closeBoxPressureRisk = 30.0;
    double boxPressureRisk = 14.0;
    double nearbyCollapseRisk = 7.0;
    double goalkeeperCloseDownRisk = 16.0;
    double eliteCarryRiskReduction = 0.25;
};

struct ShotDecisionTuning {
    double openPlayShotBaseline = 1.0;
    double pressurePenaltyScale = 1.0;
    double minimumOpenPlayXG = 0.025;
    double nonAttackingThirdMinimumXG = 0.075;
    double earlyActionMinimumXG = 0.16;
    double pressuredShotMinimumXG = 0.18;
    double weakShotXG = 0.12;
    double defensiveRoleLowXG = 0.16;
    double inappropriateRoleMinimumXG = 0.22;
    double optionScoreMinimum = 8.0;
    double strongChanceAlwaysIncludeXG = 0.30;
    double distanceScoreBaseline = 50.0;
    double distanceScoreContribution = 0.05;
    double angleScoreBaseline = 20.0;
    double angleScoreContribution = 0.04;
    double shooterConfidenceBaseline = 55.0;
    double shooterConfidenceContribution = 0.08;
    double pressurePenaltyToScoreCost = 0.26;
    double closeDefenderSmotherDistance = 1.5;
    double closeDefenderPressureDistance = 4.5;
    double closeDefenderSmotherPressure = 42.0;
    double closeDefenderOverlapCollisionDistance = 0.25;
    double closeDefenderOverlapCollisionPressure = 24.0;
    double closeDefenderPressureScale = 18.0;
    double receivedFinalBallShotContextBonus = 14.0;
    double receivedCutbackShotContextBonus = 11.0;
    double finalBallReceiverMinimumXG = 0.024;
    double clearChanceMinimumDefenderDistance = 1.75;
    double riskToleranceMinimum = 0.45;
    double riskToleranceMaximum = 1.35;
    double shotBiasBlend = 0.34;
    double shotBiasMinimum = 0.58;
    double shotBiasMaximum = 1.20;
    double longShotDistance = 24.0;
    double longShotBiasBlend = 0.40;
    double longShotBiasMinimum = 0.55;
    double longShotBiasMaximum = 1.10;
    double tacticalShotBiasBlend = 0.45;
    double tacticalShotBiasMinimum = 0.75;
    double tacticalShotBiasMaximum = 1.16;
    double weakShotBiasBlend = 0.65;
    double weakShotBiasMinimum = 0.62;
    double weakShotBiasMaximum = 1.08;
    double defensiveRoleWeakShotMultiplier = 0.45;
    double earlyActionWeakShotMultiplier = 0.58;
    double pressuredWeakShotMultiplier = 0.56;
    double belowMinimumXGPenalty = 10.0;
    double nonAttackingThirdLowXGPenalty = 12.0;
    double earlyActionLowXGPenalty = 14.0;
    double pressuredLowXGPenalty = 14.0;
    double longRangeWeakXGPenalty = 12.0;
    double defensiveRoleLowXGPenalty = 16.0;
    double xgVeryWeakThreshold = 0.02;
    double xgWeakThreshold = 0.05;
    double xgLowThreshold = 0.08;
    double xgMediumThreshold = 0.18;
    double xgGoodThreshold = 0.30;
    double xgVeryWeakBase = 2.0;
    double xgVeryWeakSlope = 150.0;
    double xgWeakBase = 8.0;
    double xgWeakSlope = 165.0;
    double xgLowBase = 18.0;
    double xgLowSlope = 165.0;
    double xgMediumBase = 34.0;
    double xgMediumSlope = 110.0;
    double xgGoodBase = 45.0;
    double xgGoodSlope = 55.0;
    double xgGreatBase = 52.0;
    double xgGreatSlope = 18.0;
};

struct ShotContextTuning {
    double blockLaneWidthMeters = 0.9;
    double lanePressureExtraWidthMeters = 0.8;
    double lanePressureReactionDistanceMeters = 24.0;
    double lanePressureDistanceWeight = 62.0;
    double lanePressureReactionWeight = 38.0;
    double minimumSegmentLengthSquared = 0.0001;
    double fallbackNearestDefenderDistance = 100.0;
};

struct ShotTypeSelectionTuning {
    double shootingSkillWeight = 0.35;
    double techniqueSkillWeight = 0.28;
    double composureSkillWeight = 0.22;
    double decisionsSkillWeight = 0.15;
    double skillBaseline = 50.0;
    double skillScale = 50.0;
    double minimumSkillLift = -0.6;
    double maximumSkillLift = 0.8;
    double pressureScale = 100.0;
    double closeShotDistance = 10.0;
    double longShotDistance = 24.0;
    double veryLongShotDistance = 32.0;
    double tightAngleCentralityScale = 4.2;
    double closeCentralMinimumCentrality = 0.22;
    double controlledFinishBaseWeight = 28.0;
    double controlledFinishCloseCentralBonus = 22.0;
    double controlledFinishSkillBonus = 8.0;
    double controlledFinishPressurePenalty = 8.0;
    double placedShotBaseWeight = 26.0;
    double placedShotSkillBonus = 12.0;
    double placedShotCentralityBonus = 12.0;
    double powerShotBaseWeight = 18.0;
    double powerShotPressureBonus = 8.0;
    double powerShotSkillBonus = 5.0;
    double longShotBaseWeight = 4.0;
    double longShotDistanceBonus = 30.0;
    double veryLongShotDistanceBonus = 16.0;
    double tightAngleShotBaseWeight = 2.0;
    double tightAngleShotTightAngleBonus = 36.0;
    double desperationShotBaseWeight = 2.0;
    double desperationShotPressureBonus = 12.0;
    double desperationShotVeryLongBonus = 10.0;
    double desperationShotLanePressureBonus = 0.08;
    double controlledFinishDifficulty = 12.0;
    double placedShotDifficulty = 18.0;
    double powerShotDifficulty = 24.0;
    double longShotDifficulty = 34.0;
    double tightAngleShotDifficulty = 38.0;
    double desperationShotDifficulty = 48.0;
};

struct ShotTargetSelectionTuning {
    double shootingSkillWeight = 0.30;
    double techniqueSkillWeight = 0.25;
    double composureSkillWeight = 0.25;
    double decisionsSkillWeight = 0.20;
    double skillBaseline = 50.0;
    double skillScale = 50.0;
    double minimumSkillLift = -0.7;
    double maximumSkillLift = 0.9;
    double pressureScale = 100.0;
    double tightAngleCentralityScale = 4.0;
    double nearPostBaseWeight = 24.0;
    double nearPostTightAngleBonus = 18.0;
    double nearPostPressureBonus = 8.0;
    double centerBaseWeight = 18.0;
    double centerSkillPenalty = 5.0;
    double centerPressureBonus = 8.0;
    double farPostBaseWeight = 30.0;
    double farPostSkillBonus = 16.0;
    double farPostCentralityBonus = 10.0;
    double farPostTightAnglePenalty = 6.0;
    double lowHeightBaseWeight = 34.0;
    double lowHeightSkillBonus = 10.0;
    double midHeightBaseWeight = 24.0;
    double midHeightPressureBonus = 6.0;
    double highHeightBaseWeight = 15.0;
    double highHeightPowerShotBonus = 8.0;
    double highHeightSkillPenalty = 4.0;
    double goalLaneOffsetShare = 0.72;
    double lowTargetHeightMeters = 0.35;
    double midTargetHeightMeters = 1.15;
    double highTargetHeightMeters = 1.95;
    double controlledFinishTargetDifficulty = 8.0;
    double placedShotTargetDifficulty = 18.0;
    double powerShotTargetDifficulty = 15.0;
    double longShotTargetDifficulty = 30.0;
    double tightAngleShotTargetDifficulty = 34.0;
    double desperationShotTargetDifficulty = 42.0;
    double centerLaneDifficulty = 4.0;
    double postLaneDifficulty = 10.0;
    double lowHeightDifficulty = 6.0;
    double midHeightDifficulty = 4.0;
    double highHeightDifficulty = 12.0;
    double placementPressurePenalty = 0.24;
    double distanceDifficultyWeight = 0.55;
    double tightAngleDifficultyWeight = 16.0;
};

struct ShotExecutionTuning {
    double shootingSkillWeight = 0.34;
    double techniqueSkillWeight = 0.26;
    double composureSkillWeight = 0.19;
    double decisionsSkillWeight = 0.13;
    double agilitySkillWeight = 0.08;
    double controlledFinishDifficulty = 10.0;
    double placedShotDifficulty = 17.0;
    double powerShotDifficulty = 23.0;
    double longShotDifficulty = 34.0;
    double tightAngleShotDifficulty = 38.0;
    double desperationShotDifficulty = 48.0;
    double pressureDifficultyWeight = 0.32;
    double distanceDifficultyWeight = 0.42;
    double angleDifficultyWeight = 24.0;
    double lanePressureDifficultyWeight = 0.12;
    double executionQualityBase = 62.0;
    double skillDifficultyBlend = 0.72;
    double executionNoiseRange = 9.0;
    double minimumExecutionQuality = 1.0;
    double maximumExecutionQuality = 99.0;
    double baseDeviationMeters = 0.35;
    double qualityDeviationScale = 4.1;
    double frameBaseLateralDeviationMeters = 0.85;
    double frameQualityLateralDeviationScale = 3.10;
    double framePressureLateralDeviationScale = 1.20;
    double frameDistanceLateralDeviationScale = 0.080;
    double frameAngleLateralDeviationScale = 1.20;
    double frameTypeLateralDeviationScale = 1.05;
    double frameBaseHeightDeviationMeters = 0.46;
    double frameQualityHeightDeviationScale = 1.70;
    double framePressureHeightDeviationScale = 0.62;
    double frameDistanceHeightDeviationScale = 0.034;
    double frameTypeHeightDeviationScale = 0.56;
    double highXGDeviationReduction = 0.0;
    double highXGDeviationReference = 0.55;
    double pressureDeviationScale = 1.35;
    double distanceDeviationScale = 0.065;
    double angleDeviationScale = 1.15;
    double typeDeviationScale = 0.90;
    double difficultyScale = 100.0;
    double laneShiftThreshold = 1.55;
    double heightShiftThreshold = 0.85;
    double heightErrorBase = 0.35;
    double heightErrorQualityScale = 1.8;
    double minimumShotPower = 18.0;
    double maximumShotPower = 34.0;
    double powerBase = 24.0;
    double powerSkillBaseline = 50.0;
    double shootingPowerWeight = 0.045;
    double techniquePowerWeight = 0.025;
    double powerShotPowerBonus = 4.0;
    double longShotPowerBonus = 4.0;
    double controlledFinishPowerPenalty = 1.8;
    double placedShotPowerPenalty = 1.8;
    double powerNoiseRange = 2.5;
    double placementExecutionBlend = 0.35;
    double placementExecutionBaseline = 55.0;
    double cleanStrikeThreshold = 58.0;
};

struct ShotQualityTuning {
    double openPlayXGIntercept = -1.25;
    double openPlayXGDistanceCoefficient = 0.130;
    double openPlayXGAngleCoefficient = 1.55;
    double openPlayXGPressureCoefficient = 0.012;
    double openPlayXGMinimum = 0.004;
    double pressureScale = 100.0;
    double lanePressureScale = 100.0;
    double preShotLanePressurePenalty = 0.22;
    double preShotNearestDefenderPenalty = 0.12;
    double tightAngleCentralityScale = 4.0;
    double longShotTypePenalty = 0.10;
    double tightAngleShotTypePenalty = 0.12;
    double desperationShotTypePenalty = 0.18;
    double powerShotTypePenalty = 0.04;
    double keeperFacingXGPressurePenalty = 0.30;
    double keeperFacingXGTightAnglePenalty = 0.18;
    double xGMinimum = 0.003;
    double blockRiskBase = 0.02;
    double blockRiskLanePressureWeight = 0.13;
    double blockRiskPressureWeight = 0.05;
    double nearestDefenderDistanceReference = 22.0;
    double nearestDefenderDistanceScale = 45.0;
    double nearestDefenderRiskMaximum = 0.10;
    double blockRiskPowerBaseline = 24.0;
    double blockRiskPowerReduction = 0.006;
    double blockRiskMinimum = 0.01;
    double blockRiskMaximum = 0.26;
    double onTargetDifficultyBase = 18.0;
    double onTargetDifficultyDistanceWeight = 1.25;
    double onTargetDifficultyPressureWeight = 0.34;
    double onTargetDifficultyTightAngleWeight = 30.0;
    double onTargetDifficultyTypeWeight = 70.0;
    double onTargetDifficultyDeviationWeight = 6.0;
    double onTargetDifficultyHeightErrorWeight = 7.0;
    double saveDifficultyBase = 34.0;
    double saveDifficultyAdjustedXGWeight = 92.0;
    double saveDifficultyPlacementWeight = 0.18;
    double saveDifficultyPowerBaseline = 22.0;
    double saveDifficultyPowerWeight = 1.4;
    double saveDifficultyTightAngleWeight = 8.0;
    double reboundRiskBase = 0.18;
    double reboundRiskPowerBaseline = 22.0;
    double reboundRiskPowerWeight = 0.018;
    double reboundRiskGoalkeeperWeaknessWeight = 0.18;
    double reboundRiskPressureWeight = 0.08;
    double reboundRiskMinimum = 0.05;
    double reboundRiskMaximum = 0.62;
};

struct ShotOutcomeTuning {
    double woodworkToleranceMeters = 0.16;
    double goalkeeperShotStoppingWeight = 0.36;
    double goalkeeperOneOnOnesWeight = 0.22;
    double goalkeeperHandlingWeight = 0.18;
    double goalkeeperAerialAbilityWeight = 0.08;
    double goalkeeperPositioningWeight = 0.08;
    double goalkeeperConcentrationWeight = 0.08;
    double goalkeeperFallbackStrengthWeight = 0.10;
    double saveSkillBaseline = 55.0;
    double goalProbabilityKeeperFacingWeight = 0.82;
    double keeperSkillGoalReductionWeight = 0.0035;
    double placementGoalWeight = 0.03;
    double powerGoalWeight = 0.02;
    double framePlacementGoalWeight = 0.03;
    double saveDifficultyGoalWeight = 0.0;
    double keeperLateralReachMeters = 2.6;
    double keeperVerticalReachMeters = 2.15;
    double keeperReachGoalWeight = 0.04;
    double goalProbabilityMinimum = 0.02;
    double goalProbabilityMaximum = 0.92;
    double shotPowerBaseline = 18.0;
    double shotPowerScale = 18.0;
    double heldReboundBase = 0.48;
    double handlingBaseline = 50.0;
    double handlingHeldScale = 0.38;
    double keeperSkillHeldScale = 0.18;
    double reboundPowerScale = 0.22;
    double reboundRiskHeldPenalty = 0.28;
    double heldMinimumProbability = 0.12;
    double heldMaximumProbability = 0.88;
};

struct ShotBlockTuning {
    double blockLaneWidthMeters = 0.75;
    double extraBlockLaneWidthMeters = 0.85;
    double blockReactionWindowSeconds = 0.42;
    double baseBlockProbability = 0.05;
    double maximumBlockProbability = 0.58;
    double defenderPositioningWeight = 0.28;
    double defenderConcentrationWeight = 0.22;
    double defenderMarkingWeight = 0.18;
    double defenderTacklingWeight = 0.16;
    double defenderAgilityWeight = 0.10;
    double defenderOverallWeight = 0.06;
    double minimumSegmentLengthSquared = 0.0001;
    double defenderSkillBaseline = 50.0;
    double defenderSkillScale = 100.0;
    double minimumReactionAdjustment = -0.12;
    double maximumReactionAdjustment = 0.24;
    double defenderInterventionSpeed = 6.5;
    double timingMinimumWindowSeconds = 0.05;
    double shotPowerSpeedPenaltyBaseline = 18.0;
    double shotPowerSpeedPenaltyScale = 26.0;
    double maximumSpeedPenalty = 0.45;
    double qualityBlockRiskWeight = 0.44;
    double laneFactorWeight = 0.20;
    double timingFactorWeight = 0.15;
    double skillFactorWeight = 0.12;
    double lanePressureWeight = 0.10;
    double speedPenaltyWeight = 0.18;
    double deflectionStrengthBase = 0.35;
    double deflectionStrengthPowerBaseline = 18.0;
    double deflectionStrengthPowerScale = 30.0;
    double deflectionStrengthLaneWeight = 0.20;
    double minimumDeflectionStrength = 0.25;
    double maximumDeflectionStrength = 0.95;
    double minimumBlockPathProgress = 0.04;
    double maximumBlockPathProgress = 0.96;
    double groundBlockReachHeightMeters = 1.15;
    double jumpingReachHeightBaseMeters = 1.25;
    double jumpingReachHeightScaleMeters = 1.25;
    double emergencyReachMarginMeters = 0.20;
};

struct ReboundModelTuning {
    double savedReboundBaseDistance = 5.0;
    double blockedDeflectionBaseDistance = 7.0;
    double savedReboundPowerDistanceScale = 12.0;
    double blockedDeflectionPowerDistanceScale = 14.0;
    double handlingDistanceReductionScale = 8.0;
    double randomDistanceScale = 7.0;
    double savedReboundAngleSpread = 0.95;
    double blockedDeflectionAngleSpread = 1.25;
    double reboundBackwardAngleRadians = 3.14159265358979323846;
    double savedReboundLateralAngleRadians = 1.57079632679489661923;
    double savedReboundBackwardBlend = 0.35;
    double shotPowerBaseline = 18.0;
    double shotPowerScale = 18.0;
    double deflectionStrengthDistanceScale = 6.0;
    double deflectionStrengthSpeedScale = 3.0;
    double minimumReboundSpeed = 1.0;
    double reboundSpeedBase = 7.0;
    double reboundSpeedPowerScale = 8.0;
    double savedReboundApexBase = 0.45;
    double savedReboundApexPowerScale = 0.9;
    double blockedDeflectionApexBase = 1.4;
    double blockedDeflectionApexPowerScale = 1.0;
};

struct ShotTrajectoryTuning {
    double minimumShotSpeedMetersPerSecond = 1.0;
    double lowShotApexMeters = 0.45;
    double midShotApexMeters = 1.15;
    double highShotApexMeters = 2.25;
};

struct ShotFlowTuning {
    double offTargetRestartSeconds = 18.0;
    double savedHeldRestartSeconds = 10.0;
    double saveContactGoalLineOffsetMeters = 1.2;
    double saveContactGoalkeeperBlend = 0.35;
    double saveContactLateralPaddingMeters = 1.5;
    double minimumSaveContactDeltaX = 0.001;
};

struct ShootingModelTuning {
    double defaultAttribute = 50.0;
    ShotContextTuning context;
    ShotTypeSelectionTuning typeSelection;
    ShotTargetSelectionTuning targetSelection;
    ShotExecutionTuning execution;
    ShotQualityTuning quality;
    ShotOutcomeTuning outcome;
    ShotBlockTuning block;
    ShotTrajectoryTuning trajectory;
    ReboundModelTuning rebound;
    ShotFlowTuning flow;
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

struct PassActionScoringProfile {
    double safetyToRetention = 0.24;
    double progressionToProgression = 0.24;
    double chancePassProgressionToChance = 0.12;
    double switchPlayTacticalFit = 5.0;
    double directPassTacticalFit = 3.0;
    double chancePassRoleFit = 4.0;
    double retentionPassRoleFit = 4.0;
    double executionDifficultySkillBase = 22.0;
    double executionDifficultyToSkillCost = 0.20;
    double laneRiskToPressureCost = 0.12;
    double receiverPressureToPressureCost = 0.08;
    double executionDifficultyToTurnoverRisk = 0.08;
    double riskToleranceMinimum = 0.45;
};

struct CarryActionScoringProfile {
    double spaceToRetention = 0.20;
    double progressionToProgression = 0.30;
    double safeCarryTacticalFit = 4.0;
    double riskCarryTacticalFit = 5.0;
    double dribbleRoleFit = 4.0;
    double progressionRoleFit = 4.0;
    double controlDifficultySkillBase = 20.0;
    double controlDifficultyToSkillCost = 0.16;
    double pressureRiskToPressureCost = 0.14;
    double controlDifficultyToTurnoverRisk = 0.06;
    double zoneLimitToTurnoverRisk = 0.16;
    double riskToleranceMinimum = 0.45;
};

struct ShotActionScoringProfile {
    double xgToChanceValue = 24.0;
    double angleToChanceValue = 0.08;
    double distanceToChanceValue = 0.08;
    double tacticalChanceFit = 2.0;
    double roleChanceFit = 4.0;
    double shooterConfidenceToSkillFit = 0.08;
    double pressurePenaltyToPressureCost = 0.22;
    double safeCirculationShotCost = 10.0;
    double earlyActionShotCost = 6.0;
    double nonClearChanceXG = 0.16;
    double riskToleranceMinimum = 0.45;
};

struct HoldActionScoringProfile {
    double retentionBase = 18.0;
    double tacticalRetentionFit = 5.0;
    double roleRetentionFit = 4.0;
    double skillBase = 8.0;
    double localPressureCost = 0.45;
};

struct ClearActionScoringProfile {
    double retentionBase = 12.0;
    double progressionBase = 22.0;
    double defensiveMentalityTacticalFit = 10.0;
    double normalTacticalFit = 4.0;
    double roleFit = 8.0;
    double skillBase = 8.0;
    double localPressureCost = 0.20;
    double targetAdvanceMeters = 35.0;
};

struct PhaseFitProfile {
    double shotBoxEntry = 6.0;
    double shotChanceCreation = 5.0;
    double shotFinalThird = 3.0;
    double shotBuildUp = -14.0;
    double shotStalledFinalThird = 38.0;
    int shotStalledActionCount = 5;
    double shotStalledSeconds = 14.0;
    double carryProgressionAvailable = 4.0;
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
    double shotSkillBaseline = 0.30;
    double passSkillMinimum = 0.78;
    double passSkillMaximum = 1.18;
    double carrySkillMinimum = 0.82;
    double carrySkillMaximum = 1.14;
    double shotSkillMinimum = 0.12;
    double shotSkillMaximum = 0.82;
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

inline PassActionScoringProfile defaultPassActionScoringProfile() {
    return PassActionScoringProfile{};
}

inline CarryActionScoringProfile defaultCarryActionScoringProfile() {
    return CarryActionScoringProfile{};
}

inline ShotActionScoringProfile defaultShotActionScoringProfile() {
    return ShotActionScoringProfile{};
}

inline HoldActionScoringProfile defaultHoldActionScoringProfile() {
    return HoldActionScoringProfile{};
}

inline ClearActionScoringProfile defaultClearActionScoringProfile() {
    return ClearActionScoringProfile{};
}

inline PhaseFitProfile defaultPhaseFitProfile() {
    return PhaseFitProfile{};
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
        profile.shotBias += 0.08;
        profile.weakShotBias -= 0.02;
    }

    if (tactics.tempo == TeamTempo::Low) {
        profile.shotBias -= 0.06;
        profile.weakShotBias -= 0.12;
    } else if (tactics.tempo == TeamTempo::High) {
        profile.shotBias += 0.03;
        profile.weakShotBias -= 0.02;
    }

    if (tactics.passingDirectness == PassingDirectness::Direct) {
        profile.shotBias += 0.05;
    } else if (tactics.passingDirectness == PassingDirectness::Short) {
        profile.weakShotBias -= 0.08;
    }

    profile.shotBias = std::clamp(profile.shotBias, 0.55, 1.35);
    profile.weakShotBias = std::clamp(profile.weakShotBias, 0.45, 1.05);
    return profile;
}
