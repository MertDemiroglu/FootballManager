#pragma once

#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>
#include<vector>

enum class ShotType {
    ControlledFinish,
    PlacedShot,
    PowerShot,
    LongShot,
    TightAngleShot,
    DesperationShot
};

enum class ShotTargetLane {
    NearPost,
    Center,
    FarPost
};

enum class ShotTargetHeight {
    Low,
    Mid,
    High
};

struct ShotTargetZone {
    ShotTargetLane lane = ShotTargetLane::Center;
    ShotTargetHeight height = ShotTargetHeight::Mid;
};

struct ShotContext {
    PitchPoint shotOrigin;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    double distanceMeters = 0.0;
    double angleRadians = 0.0;
    double centrality = 0.0;
    double pressure = 0.0;
    double lanePressure = 0.0;
    double nearestDefenderDistance = 100.0;
    PlayerAttributes shooterAttributes;
    PlayerAttributes goalkeeperAttributes;
    double goalkeeperStrength = 50.0;
    std::uint64_t seed = 0;
};

struct ShotContextBuildRequest {
    PitchPoint shotOrigin;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    double pressure = 0.0;
    PlayerAttributes shooterAttributes;
    PlayerAttributes goalkeeperAttributes;
    double goalkeeperStrength = 50.0;
    std::vector<PitchPoint> defenderPositions;
    std::uint64_t seed = 0;
};

struct ShotTypeSelectionResult {
    ShotType type = ShotType::ControlledFinish;
    double typeDifficulty = 0.0;
};

struct ShotTargetSelectionResult {
    ShotTargetZone intendedZone;
    PitchPoint intendedTarget;
    double targetDifficulty = 0.0;
    double placementQuality = 0.0;
};

struct ShotExecutionResult {
    ShotTargetZone actualZone;
    PitchPoint actualTarget;
    double executionQuality = 0.0;
    double targetDeviationMeters = 0.0;
    double heightError = 0.0;
    double shotPower = 0.0;
    double placementQuality = 0.0;
    bool cleanStrike = false;
};

struct ShotQualityResult {
    double baseXG = 0.0;
    double adjustedXG = 0.0;
    double blockRisk = 0.0;
    double onTargetDifficulty = 0.0;
    double saveDifficulty = 0.0;
    double reboundRisk = 0.0;
};
