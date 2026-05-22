#include"fm/roster/Forward.h"

#include"fm/roster/PlayerOverallCalculator.h"

Forward::Forward(const std::string& name, PlayerPosition position, const std::string& team, int age, int f, int d, int dr, int l, int ft)
    : Footballer(name, position, team, age) {
    setAttributes(buildAttributesFromLegacySkills(position, f, d, dr, l, ft, age));
}

Forward::Forward(const std::string& name, const std::string& team, int age, int f, int d, int dr, int l, int ft)
    : Forward(name, PlayerPosition::Striker, team, age, f, d, dr, l, ft) {}

int Forward::totalPower() const {
    return PlayerOverallCalculator::calculateOverall(attributes, position);
}
double Forward::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}
