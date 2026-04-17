#pragma once
#include "Footballer.h"

class Goalkeeper : public Footballer {
private:
    int reflexes, diving, aerial, positioning, kicking;

public:
    Goalkeeper(const std::string& name, const std::string& team, int age, int reflexes, int diving, int aerial, int positioning, int kicking);

    int totalPower() const override;
    double calculateMarketValue() const override;
};