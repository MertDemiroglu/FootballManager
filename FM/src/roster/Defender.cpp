#include"fm/roster/Defender.h"

#include"fm/roster/PlayerOverallCalculator.h"

Defender::Defender(const std::string& name, PlayerPosition position, const std::string& team, int age, int t, int c, int p, int h, int co)
    : Footballer(name, position, team, age) {
    setAttributes(buildAttributesFromLegacySkills(position, t, c, p, h, co, age));
}

Defender::Defender(const std::string& name, const std::string& team, int age, int t, int c, int p, int h, int co)
    : Defender(name, PlayerPosition::CenterBack, team, age, t, c, p, h, co) {}

int Defender::totalPower() const {
    return PlayerOverallCalculator::calculateOverall(attributes, position);
}
double Defender::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}
