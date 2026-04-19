#pragma once

#include "fm/common/Types.h"
#include "fm/roster/Formation.h"

#include <string>
#include <vector>

struct CoachSeedData {
    CoachId id = 0;
    std::string name;
    FormationId preferredFormation = FormationId::FourFourTwo;
};

struct PlayerSeedData {
    PlayerId id = 0;
    TeamId teamId = 0;
    std::string name;
    std::string position;
    int age = 0;
    Money wage = 0;
    int contractYears = 0;
    int s1 = 0;
    int s2 = 0;
    int s3 = 0;
    int s4 = 0;
    int s5 = 0;
};

struct TeamSeedData {
    TeamId id = 0;
    LeagueId leagueId = 0;
    std::string name;
    Money transferBudget = 0;
    Money wageBudget = 0;
    Money totalBudget = 0;
    CoachSeedData coach;
    std::vector<PlayerSeedData> players;
};

struct LeagueSeedData {
    LeagueId id = 0;
    std::string name;
    std::vector<TeamSeedData> teams;
};
