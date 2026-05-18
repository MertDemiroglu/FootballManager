#pragma once

#include "fm/roster/PlayerPosition.h"

struct TechnicalAttributes {
    int shooting = 0;
    int passing = 0;
    int firstTouch = 0;
    int technique = 0;
    int tackling = 0;
    int dribbling = 0;
    int crossing = 0;
    int marking = 0;
    int heading = 0;
    int setPieces = 0;
};

struct MentalAttributes {
    int decisions = 0;
    int vision = 0;
    int positioning = 0;
    int offTheBall = 0;
    int composure = 0;
    int concentration = 0;
    int workRate = 0;
    int teamwork = 0;
    int leadership = 0;
    int aggression = 0;
};

struct PhysicalAttributes {
    int pace = 0;
    int acceleration = 0;
    int stamina = 0;
    int strength = 0;
    int agility = 0;
    int jumpingReach = 0;
};

struct GoalkeeperAttributes {
    int shotStopping = 0;
    int handling = 0;
    int aerialAbility = 0;
    int oneOnOnes = 0;
    int distribution = 0;
};

struct PlayerAttributes {
    TechnicalAttributes technical;
    MentalAttributes mental;
    PhysicalAttributes physical;
    GoalkeeperAttributes goalkeeper;
};

bool isValidAttributeValue(int value);
void validatePlayerAttributes(const PlayerAttributes& attributes);
PlayerAttributes clampPlayerAttributes(PlayerAttributes attributes);

double averageTechnical(const PlayerAttributes& attributes);
double averageMental(const PlayerAttributes& attributes);
double averagePhysical(const PlayerAttributes& attributes);
double averageGoalkeeper(const PlayerAttributes& attributes);

PlayerAttributes buildAttributesFromLegacySkills(
    PlayerPosition position,
    int s1,
    int s2,
    int s3,
    int s4,
    int s5,
    int age);
