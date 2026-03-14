#pragma once

#include"LeagueRules.h"
#include"SeasonPlan.h"

class League;

class FixtureGenerator {
public:
    void generateSeasonFixture(League& league, const SeasonPlan& plan, const LeagueRules& rules ) const;
};