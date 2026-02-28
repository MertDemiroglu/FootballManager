#pragma once

#include "Date.h"

class League;

class FixtureGenerator {
public:
    void generateSeasonFixture(League& league, const Date& startDate) const;
};