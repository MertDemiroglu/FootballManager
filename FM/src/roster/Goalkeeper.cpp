#include"fm/roster/Goalkeeper.h"

#include"fm/roster/PlayerOverallCalculator.h"

Goalkeeper::Goalkeeper(const std::string& name, const std::string& team, int age, int r, int d, int a, int p, int k)
    : Footballer(name, PlayerPosition::Goalkeeper, team, age) {
    setAttributes(buildAttributesFromLegacySkills(PlayerPosition::Goalkeeper, r, d, a, p, k, age));
}

int Goalkeeper::totalPower() const {
    return PlayerOverallCalculator::calculateOverall(attributes, position);
}
double Goalkeeper::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}
