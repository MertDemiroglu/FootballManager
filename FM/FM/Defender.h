#pragma once
#include "Footballer.h"

class Defender : public Footballer {
private:
    int tackling, concentraiton, positioning, heading, composure;

public:
    Defender(const std::string& name, PlayerPosition position, const std::string& team, int age, int tackling, int concentraiton, int positioning, int heading, int composure);
    Defender(const std::string& name, const std::string& team, int age, int tackling, int concentraiton, int positioning, int heading, int composure);

    int totalPower() const override;
    double calculateMarketValue() const override;
};
