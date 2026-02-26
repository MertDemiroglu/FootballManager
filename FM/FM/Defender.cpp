#include"Defender.h"

Defender::Defender(const std::string& name, const std::string& team, int age, int t, int c, int p, int h, int co) : Footballer(name, "Defender", team, age), tackling(t), concentraiton(c), positioning(p), heading(h), composure(co){}

int Defender::totalPower() const {
    return (tackling * 3 + concentraiton * 2 + positioning * 2 + heading + composure) / 9;
}
double Defender::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}