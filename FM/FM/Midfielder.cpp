#include"Midfielder.h"

Midfielder::Midfielder(const std::string& name, const std::string& team, int age, int c, int v, int p, int d, int t) : Footballer(name, "Midfielder", team, age), crossing(c), vision(v), passing(p), decision(d), technique(t){}

int Midfielder::totalPower() const {
    return (crossing * 3 + vision * 2 + passing * 2 + decision + technique * 2) / 9;
}
double Midfielder::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}