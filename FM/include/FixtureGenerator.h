#pragma once
#include<functional>

#include"LeagueRules.h"
#include"SeasonPlan.h"
#include"fm/common/Types.h"
class League;

class FixtureGenerator {
public:
    void generateSeasonFixture(League& league, const SeasonPlan& plan, const LeagueRules& rules, const std::function<MatchId()>& matchIdAllocator) const;
};