#include"Midfielder.h"

Midfielder::Midfielder(const std::string& name, PlayerPosition position, const std::string& team, int age, int c, int v, int p, int d, int t)
    : Footballer(name, position, team, age), crossing(c), vision(v), passing(p), decision(d), technique(t) {}

Midfielder::Midfielder(const std::string& name, const std::string& team, int age, int c, int v, int p, int d, int t)
    : Midfielder(name, PlayerPosition::CentralMidfielder, team, age, c, v, p, d, t) {}

int Midfielder::totalPower() const {
    return (crossing * 3 + vision * 2 + passing * 2 + decision + technique * 2) / 9;
}
double Midfielder::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}
