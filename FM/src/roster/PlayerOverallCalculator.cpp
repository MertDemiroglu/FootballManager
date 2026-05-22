#include "fm/roster/PlayerOverallCalculator.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace {
    struct WeightedAttribute {
        int value = 0;
        double weight = 1.0;
    };

    int weightedAverage(std::initializer_list<WeightedAttribute> attributes) {
        double weightedTotal = 0.0;
        double totalWeight = 0.0;

        for (const WeightedAttribute& attribute : attributes) {
            weightedTotal += static_cast<double>(attribute.value) * attribute.weight;
            totalWeight += attribute.weight;
        }

        if (totalWeight <= 0.0) {
            return 0;
        }

        return std::clamp(static_cast<int>(std::lround(weightedTotal / totalWeight)), 0, 100);
    }
}

int PlayerOverallCalculator::calculateOverall(const PlayerAttributes& attributes, PlayerPosition position) {
    const TechnicalAttributes& t = attributes.technical;
    const MentalAttributes& m = attributes.mental;
    const PhysicalAttributes& p = attributes.physical;
    const GoalkeeperAttributes& g = attributes.goalkeeper;

    switch (position) {
    case PlayerPosition::Goalkeeper:
        return weightedAverage({
            { g.shotStopping, 2.0 },
            { g.handling, 1.6 },
            { g.aerialAbility, 1.4 },
            { g.oneOnOnes, 1.4 },
            { g.distribution, 0.9 },
            { m.positioning, 1.1 },
            { m.concentration, 1.1 },
            { m.composure, 0.8 },
            { p.agility, 0.9 },
            { p.jumpingReach, 0.8 }
        });

    case PlayerPosition::CenterBack:
        return weightedAverage({
            { t.marking, 1.8 },
            { t.tackling, 1.8 },
            { t.heading, 1.3 },
            { p.strength, 1.2 },
            { m.positioning, 1.5 },
            { m.concentration, 1.2 },
            { p.jumpingReach, 0.9 },
            { t.passing, 0.7 }
        });

    case PlayerPosition::LeftBack:
    case PlayerPosition::RightBack:
        return weightedAverage({
            { t.crossing, 1.2 },
            { t.tackling, 1.4 },
            { t.marking, 1.2 },
            { p.pace, 1.2 },
            { p.stamina, 1.2 },
            { p.acceleration, 1.0 },
            { m.workRate, 1.1 },
            { t.passing, 0.8 }
        });

    case PlayerPosition::DefensiveMidfielder:
        return weightedAverage({
            { t.tackling, 1.5 },
            { t.marking, 1.2 },
            { m.positioning, 1.5 },
            { t.passing, 1.2 },
            { m.decisions, 1.1 },
            { p.stamina, 1.0 },
            { p.strength, 0.8 },
            { m.teamwork, 0.9 }
        });

    case PlayerPosition::CentralMidfielder:
        return weightedAverage({
            { t.passing, 1.5 },
            { t.firstTouch, 1.2 },
            { m.decisions, 1.3 },
            { m.vision, 1.1 },
            { p.stamina, 1.0 },
            { m.teamwork, 1.0 },
            { t.technique, 1.1 },
            { m.workRate, 0.8 }
        });

    case PlayerPosition::AttackingMidfielder:
        return weightedAverage({
            { t.dribbling, 1.3 },
            { t.passing, 1.2 },
            { t.technique, 1.3 },
            { t.firstTouch, 1.2 },
            { m.vision, 1.2 },
            { m.offTheBall, 1.0 },
            { t.shooting, 0.8 },
            { p.acceleration, 0.8 }
        });

    case PlayerPosition::LeftMidfielder:
    case PlayerPosition::RightMidfielder:
    case PlayerPosition::LeftWinger:
    case PlayerPosition::RightWinger:
        return weightedAverage({
            { t.dribbling, 1.4 },
            { t.crossing, 1.3 },
            { t.passing, 1.0 },
            { t.technique, 1.1 },
            { t.firstTouch, 0.9 },
            { m.vision, 0.7 },
            { m.offTheBall, 1.0 },
            { p.pace, 1.1 },
            { p.acceleration, 1.0 }
        });

    case PlayerPosition::Striker:
        return weightedAverage({
            { t.shooting, 2.0 },
            { m.offTheBall, 1.4 },
            { m.composure, 1.4 },
            { t.firstTouch, 1.1 },
            { t.heading, 0.9 },
            { p.pace, 0.9 },
            { t.technique, 0.9 },
            { p.strength, 0.7 }
        });

    case PlayerPosition::Unknown:
        break;
    }

    return 0;
}
