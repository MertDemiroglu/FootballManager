#include"fm/roster/Forward.h"

Forward::Forward(const std::string& name, PlayerPosition position, const std::string& team, int age, int f, int d, int dr, int l, int ft)
    : Footballer(name, position, team, age), finishing(f), determination(d), dribbling(dr), longShots(l), firstTouch(ft) {}

Forward::Forward(const std::string& name, const std::string& team, int age, int f, int d, int dr, int l, int ft)
    : Forward(name, PlayerPosition::Striker, team, age, f, d, dr, l, ft) {}

int Forward::totalPower() const {
    return (finishing * 3 + determination * 2 + dribbling * 2 + longShots + firstTouch) / 9;
}
double Forward::calculateMarketValue() const {
    return totalPower() * 1.8 + (30 - age) * 2;
}
