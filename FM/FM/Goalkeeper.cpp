#include"Goalkeeper.h"

Goalkeeper::Goalkeeper(const std::string& name, const std::string& team, int age, int r, int d, int a, int p, int k) : Footballer(name, "Goalkeeper", team, age),  reflexes(r), diving(d), aerial(a), positioning(p), kicking(k){}

int Goalkeeper::totalPower() const {
    return (reflexes * 3 + diving * 2 + aerial * 2 + positioning + kicking) / 9;
}
double Goalkeeper::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}