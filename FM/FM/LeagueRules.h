#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Date.h"

struct TransferWindow {
    Date start;
    Date endExclusive;

    bool contains(const Date& date) const;
};

struct LeagueRules {
    std::string leagueId;
    std::string leagueName;

    int teamCount = 18;
    int matchdaysPerSeason = 34;
    int firstHalfRounds = 17;

    std::function<Date(int)> preseasonStart;
    std::function<Date(int)> kickoffDate;
    std::function<Date(int)> nextPreseasonStart;
    std::function<TransferWindow(int)> summerWindow;
    std::function<TransferWindow(int)> winterWindow;

    bool winterBreakEnabled = true;
    int winterBreakLengthDays = 28;
    int winterBreakAfterRoundIndex = 17;

    int matchSpacingDays = 7;
    std::vector<int> matchdayDistributionOffsets;

    static Date secondSaturdayOfAugust(int year);
    static LeagueRules makeSuperLig();
};
