#pragma once
#include"fm/roster/Footballer.h"

class Midfielder : public Footballer {
private:
    int crossing, vision, passing, decision, technique;

public:
    Midfielder(const std::string& name, PlayerPosition position, const std::string& team, int age, int crossing, int vision, int passing, int decision, int techinque);
    Midfielder(const std::string& name, const std::string& team, int age, int crossing, int vision, int passing, int decision, int techinque);

    int totalPower() const override;
    double calculateMarketValue() const override;
};
