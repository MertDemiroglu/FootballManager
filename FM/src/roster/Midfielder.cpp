#include"fm/roster/Midfielder.h"

#include"fm/roster/PlayerOverallCalculator.h"

Midfielder::Midfielder(const std::string& name, PlayerPosition position, const std::string& team, int age, int c, int v, int p, int d, int t)
    : Footballer(name, position, team, age) {
    setAttributes(buildAttributesFromLegacySkills(position, c, v, p, d, t, age));
}

Midfielder::Midfielder(const std::string& name, const std::string& team, int age, int c, int v, int p, int d, int t)
    : Midfielder(name, PlayerPosition::CentralMidfielder, team, age, c, v, p, d, t) {}

int Midfielder::totalPower() const {
    return PlayerOverallCalculator::calculateOverall(attributes, position);
}
double Midfielder::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}
