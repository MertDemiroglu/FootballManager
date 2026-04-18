#pragma once
#include"fm/roster/Footballer.h"

class Forward : public Footballer {
private:
    int finishing, determination, dribbling, longShots, firstTouch;

public:
    Forward(const std::string& name, PlayerPosition position, const std::string& team, int age, int finishing, int determination, int dribbling, int longShots, int firstTouch);
    Forward(const std::string& name, const std::string& team, int age, int finishing, int determination, int dribbling, int longShots, int firstTouch);

    int totalPower() const override;
    double calculateMarketValue() const override;
};
