#include "fm/roster/PlayerAttributes.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {
    constexpr int MinAttribute = 0;
    constexpr int MaxAttribute = 100;

    int clampAttribute(int value) {
        return std::clamp(value, MinAttribute, MaxAttribute);
    }

    int legacyAverage(int s1, int s2, int s3, int s4, int s5) {
        return (s1 + s2 + s3 + s4 + s5) / 5;
    }

    void validateValue(int value, const char* name) {
        if (!isValidAttributeValue(value)) {
            throw std::invalid_argument(std::string(name) + " must be between 0 and 100");
        }
    }

    template <typename Func>
    void forEachTechnical(TechnicalAttributes& attributes, Func func) {
        func(attributes.shooting);
        func(attributes.passing);
        func(attributes.firstTouch);
        func(attributes.technique);
        func(attributes.tackling);
        func(attributes.dribbling);
        func(attributes.crossing);
        func(attributes.marking);
        func(attributes.heading);
        func(attributes.setPieces);
    }

    template <typename Func>
    void forEachMental(MentalAttributes& attributes, Func func) {
        func(attributes.decisions);
        func(attributes.vision);
        func(attributes.positioning);
        func(attributes.offTheBall);
        func(attributes.composure);
        func(attributes.concentration);
        func(attributes.workRate);
        func(attributes.teamwork);
        func(attributes.leadership);
        func(attributes.aggression);
    }

    template <typename Func>
    void forEachPhysical(PhysicalAttributes& attributes, Func func) {
        func(attributes.pace);
        func(attributes.acceleration);
        func(attributes.stamina);
        func(attributes.strength);
        func(attributes.agility);
        func(attributes.jumpingReach);
    }

    template <typename Func>
    void forEachGoalkeeper(GoalkeeperAttributes& attributes, Func func) {
        func(attributes.shotStopping);
        func(attributes.handling);
        func(attributes.aerialAbility);
        func(attributes.oneOnOnes);
        func(attributes.distribution);
    }

    double average(const std::initializer_list<int> values) {
        return static_cast<double>(std::accumulate(values.begin(), values.end(), 0)) / values.size();
    }

    int tuned(int value, int delta) {
        return clampAttribute(value + delta);
    }

    void applyAgeShape(PlayerAttributes& attributes, int age) {
        int mentalDelta = 0;
        int physicalDelta = 0;

        if (age <= 20) {
            mentalDelta = -4;
            physicalDelta = 2;
        }
        else if (age <= 23) {
            mentalDelta = -2;
            physicalDelta = 1;
        }
        else if (age >= 34) {
            mentalDelta = 3;
            physicalDelta = -5;
        }
        else if (age >= 31) {
            mentalDelta = 2;
            physicalDelta = -2;
        }
        else if (age >= 27) {
            mentalDelta = 1;
        }

        forEachMental(attributes.mental, [mentalDelta](int& value) { value = tuned(value, mentalDelta); });
        forEachPhysical(attributes.physical, [physicalDelta](int& value) { value = tuned(value, physicalDelta); });
    }

    PlayerAttributes makeOutfieldBaseline(int base) {
        PlayerAttributes attributes;
        attributes.technical.shooting = tuned(base, -10);
        attributes.technical.passing = tuned(base, -2);
        attributes.technical.firstTouch = tuned(base, -2);
        attributes.technical.technique = tuned(base, -3);
        attributes.technical.tackling = tuned(base, -8);
        attributes.technical.dribbling = tuned(base, -5);
        attributes.technical.crossing = tuned(base, -8);
        attributes.technical.marking = tuned(base, -8);
        attributes.technical.heading = tuned(base, -6);
        attributes.technical.setPieces = tuned(base, -10);

        attributes.mental.decisions = tuned(base, -2);
        attributes.mental.vision = tuned(base, -4);
        attributes.mental.positioning = tuned(base, -3);
        attributes.mental.offTheBall = tuned(base, -4);
        attributes.mental.composure = tuned(base, -3);
        attributes.mental.concentration = tuned(base, -2);
        attributes.mental.workRate = tuned(base, -2);
        attributes.mental.teamwork = tuned(base, -2);
        attributes.mental.leadership = tuned(base, -8);
        attributes.mental.aggression = tuned(base, -4);

        attributes.physical.pace = tuned(base, -4);
        attributes.physical.acceleration = tuned(base, -4);
        attributes.physical.stamina = tuned(base, -2);
        attributes.physical.strength = tuned(base, -3);
        attributes.physical.agility = tuned(base, -4);
        attributes.physical.jumpingReach = tuned(base, -5);

        attributes.goalkeeper.shotStopping = tuned(base, -35);
        attributes.goalkeeper.handling = tuned(base, -35);
        attributes.goalkeeper.aerialAbility = tuned(base, -35);
        attributes.goalkeeper.oneOnOnes = tuned(base, -35);
        attributes.goalkeeper.distribution = tuned(base, -25);
        return attributes;
    }
}

bool isValidAttributeValue(int value) {
    return value >= MinAttribute && value <= MaxAttribute;
}

void validatePlayerAttributes(const PlayerAttributes& attributes) {
    validateValue(attributes.technical.shooting, "shooting");
    validateValue(attributes.technical.passing, "passing");
    validateValue(attributes.technical.firstTouch, "firstTouch");
    validateValue(attributes.technical.technique, "technique");
    validateValue(attributes.technical.tackling, "tackling");
    validateValue(attributes.technical.dribbling, "dribbling");
    validateValue(attributes.technical.crossing, "crossing");
    validateValue(attributes.technical.marking, "marking");
    validateValue(attributes.technical.heading, "heading");
    validateValue(attributes.technical.setPieces, "setPieces");

    validateValue(attributes.mental.decisions, "decisions");
    validateValue(attributes.mental.vision, "vision");
    validateValue(attributes.mental.positioning, "positioning");
    validateValue(attributes.mental.offTheBall, "offTheBall");
    validateValue(attributes.mental.composure, "composure");
    validateValue(attributes.mental.concentration, "concentration");
    validateValue(attributes.mental.workRate, "workRate");
    validateValue(attributes.mental.teamwork, "teamwork");
    validateValue(attributes.mental.leadership, "leadership");
    validateValue(attributes.mental.aggression, "aggression");

    validateValue(attributes.physical.pace, "pace");
    validateValue(attributes.physical.acceleration, "acceleration");
    validateValue(attributes.physical.stamina, "stamina");
    validateValue(attributes.physical.strength, "strength");
    validateValue(attributes.physical.agility, "agility");
    validateValue(attributes.physical.jumpingReach, "jumpingReach");

    validateValue(attributes.goalkeeper.shotStopping, "shotStopping");
    validateValue(attributes.goalkeeper.handling, "handling");
    validateValue(attributes.goalkeeper.aerialAbility, "aerialAbility");
    validateValue(attributes.goalkeeper.oneOnOnes, "oneOnOnes");
    validateValue(attributes.goalkeeper.distribution, "distribution");
}

PlayerAttributes clampPlayerAttributes(PlayerAttributes attributes) {
    forEachTechnical(attributes.technical, [](int& value) { value = clampAttribute(value); });
    forEachMental(attributes.mental, [](int& value) { value = clampAttribute(value); });
    forEachPhysical(attributes.physical, [](int& value) { value = clampAttribute(value); });
    forEachGoalkeeper(attributes.goalkeeper, [](int& value) { value = clampAttribute(value); });
    return attributes;
}

double averageTechnical(const PlayerAttributes& attributes) {
    return average({
        attributes.technical.shooting,
        attributes.technical.passing,
        attributes.technical.firstTouch,
        attributes.technical.technique,
        attributes.technical.tackling,
        attributes.technical.dribbling,
        attributes.technical.crossing,
        attributes.technical.marking,
        attributes.technical.heading,
        attributes.technical.setPieces
    });
}

double averageMental(const PlayerAttributes& attributes) {
    return average({
        attributes.mental.decisions,
        attributes.mental.vision,
        attributes.mental.positioning,
        attributes.mental.offTheBall,
        attributes.mental.composure,
        attributes.mental.concentration,
        attributes.mental.workRate,
        attributes.mental.teamwork,
        attributes.mental.leadership,
        attributes.mental.aggression
    });
}

double averagePhysical(const PlayerAttributes& attributes) {
    return average({
        attributes.physical.pace,
        attributes.physical.acceleration,
        attributes.physical.stamina,
        attributes.physical.strength,
        attributes.physical.agility,
        attributes.physical.jumpingReach
    });
}

double averageGoalkeeper(const PlayerAttributes& attributes) {
    return average({
        attributes.goalkeeper.shotStopping,
        attributes.goalkeeper.handling,
        attributes.goalkeeper.aerialAbility,
        attributes.goalkeeper.oneOnOnes,
        attributes.goalkeeper.distribution
    });
}

PlayerAttributes buildAttributesFromLegacySkills(
    PlayerPosition position,
    int s1,
    int s2,
    int s3,
    int s4,
    int s5,
    int age) {
    const int base = legacyAverage(s1, s2, s3, s4, s5);
    PlayerAttributes attributes = makeOutfieldBaseline(base);

    switch (position) {
    case PlayerPosition::Goalkeeper:
        attributes = PlayerAttributes{};
        attributes.goalkeeper.shotStopping = tuned(s1, 2);
        attributes.goalkeeper.handling = tuned(s2, 1);
        attributes.goalkeeper.aerialAbility = tuned(s3, 1);
        attributes.goalkeeper.oneOnOnes = tuned(s4, 1);
        attributes.goalkeeper.distribution = tuned(s5, 0);
        attributes.mental.positioning = tuned(s4, 0);
        attributes.mental.concentration = tuned(s2, 0);
        attributes.mental.composure = tuned(s5, -1);
        attributes.mental.decisions = tuned(base, -2);
        attributes.mental.vision = tuned(base, -10);
        attributes.mental.teamwork = tuned(base, -4);
        attributes.mental.workRate = tuned(base, -6);
        attributes.mental.leadership = tuned(base, -8);
        attributes.mental.aggression = tuned(base, -12);
        attributes.mental.offTheBall = tuned(base, -25);
        attributes.physical.agility = tuned(s1, 0);
        attributes.physical.jumpingReach = tuned(s3, 0);
        attributes.physical.strength = tuned(base, -5);
        attributes.physical.pace = tuned(base, -18);
        attributes.physical.acceleration = tuned(base, -16);
        attributes.physical.stamina = tuned(base, -8);
        attributes.technical.passing = tuned(s5, -12);
        attributes.technical.firstTouch = tuned(base, -18);
        attributes.technical.technique = tuned(base, -18);
        attributes.technical.shooting = tuned(base, -40);
        attributes.technical.tackling = tuned(base, -35);
        attributes.technical.dribbling = tuned(base, -25);
        attributes.technical.crossing = tuned(base, -30);
        attributes.technical.marking = tuned(base, -20);
        attributes.technical.heading = tuned(base, -10);
        attributes.technical.setPieces = tuned(base, -25);
        break;

    case PlayerPosition::CenterBack:
        attributes.technical.tackling = tuned(s1, 2);
        attributes.technical.marking = tuned(s2, 2);
        attributes.technical.heading = tuned(s3, 2);
        attributes.physical.strength = tuned(s4, 1);
        attributes.mental.positioning = tuned(s5, 1);
        attributes.mental.concentration = tuned(s2, 1);
        attributes.physical.jumpingReach = tuned(s3, 1);
        attributes.technical.passing = tuned(base, -4);
        attributes.technical.shooting = tuned(base, -18);
        break;

    case PlayerPosition::LeftBack:
    case PlayerPosition::RightBack:
        attributes.technical.crossing = tuned(s1, 1);
        attributes.technical.tackling = tuned(s2, 1);
        attributes.technical.marking = tuned(s3, 0);
        attributes.physical.pace = tuned(s4, 2);
        attributes.physical.acceleration = tuned(s4, 1);
        attributes.physical.stamina = tuned(s5, 2);
        attributes.mental.workRate = tuned(s5, 1);
        attributes.technical.passing = tuned(base, -2);
        attributes.technical.shooting = tuned(base, -14);
        break;

    case PlayerPosition::DefensiveMidfielder:
        attributes.technical.tackling = tuned(s1, 1);
        attributes.technical.marking = tuned(s2, 1);
        attributes.mental.positioning = tuned(s3, 2);
        attributes.technical.passing = tuned(s4, 1);
        attributes.mental.decisions = tuned(s5, 1);
        attributes.physical.stamina = tuned(base, 1);
        attributes.physical.strength = tuned(base, 0);
        attributes.mental.teamwork = tuned(base, 1);
        attributes.technical.shooting = tuned(base, -14);
        break;

    case PlayerPosition::CentralMidfielder:
        attributes.technical.passing = tuned(s1, 2);
        attributes.technical.firstTouch = tuned(s2, 1);
        attributes.mental.decisions = tuned(s3, 2);
        attributes.mental.vision = tuned(s4, 1);
        attributes.physical.stamina = tuned(s5, 1);
        attributes.mental.teamwork = tuned(base, 1);
        attributes.technical.technique = tuned(base, 1);
        attributes.mental.workRate = tuned(base, 0);
        break;

    case PlayerPosition::AttackingMidfielder:
        attributes.technical.dribbling = tuned(s1, 1);
        attributes.technical.passing = tuned(s2, 1);
        attributes.technical.technique = tuned(s3, 2);
        attributes.technical.firstTouch = tuned(s4, 1);
        attributes.mental.vision = tuned(s5, 2);
        attributes.mental.offTheBall = tuned(base, 1);
        attributes.technical.shooting = tuned(base, -1);
        attributes.technical.crossing = tuned(base, -4);
        attributes.technical.tackling = tuned(base, -14);
        attributes.technical.marking = tuned(base, -16);
        break;

    case PlayerPosition::LeftMidfielder:
    case PlayerPosition::RightMidfielder:
    case PlayerPosition::LeftWinger:
    case PlayerPosition::RightWinger:
        attributes.physical.pace = tuned(s1, 2);
        attributes.physical.acceleration = tuned(s1, 1);
        attributes.technical.dribbling = tuned(s2, 2);
        attributes.technical.crossing = tuned(s3, 2);
        attributes.technical.technique = tuned(s4, 1);
        attributes.mental.offTheBall = tuned(s5, 1);
        attributes.technical.passing = tuned(base, -2);
        attributes.technical.shooting = tuned(base, -4);
        attributes.technical.tackling = tuned(base, -16);
        attributes.technical.marking = tuned(base, -18);
        break;

    case PlayerPosition::Striker:
        attributes.technical.shooting = tuned(s1, 3);
        attributes.mental.offTheBall = tuned(s2, 2);
        attributes.mental.composure = tuned(s3, 2);
        attributes.technical.firstTouch = tuned(s4, 1);
        attributes.technical.heading = tuned(s5, 1);
        attributes.physical.pace = tuned(base, 0);
        attributes.technical.technique = tuned(base, -1);
        attributes.physical.strength = tuned(base, -1);
        attributes.technical.tackling = tuned(base, -25);
        attributes.technical.marking = tuned(base, -24);
        break;

    case PlayerPosition::Unknown:
        break;
    }

    applyAgeShape(attributes, age);
    return clampPlayerAttributes(attributes);
}
