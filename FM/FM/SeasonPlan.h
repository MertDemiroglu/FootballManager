#pragma once

#include <optional>

#include "Date.h"
#include "LeagueRules.h"

class League;

class SeasonPlan {
private:
    int seasonYear = 0;
    Date preseasonStart = Date(1970, Month::January, 1);
    Date kickoff = Date(1970, Month::January, 1);
    Date nextPreseasonStart = Date(1971, Month::January, 1);
    TransferWindow summerWindow = TransferWindow{ Date(1970, Month::January, 1), Date(1970, Month::January, 2) };
    TransferWindow winterWindow = TransferWindow{ Date(1970, Month::January, 1), Date(1970, Month::January, 2) };

    std::optional<Date> winterBreakStart;
    std::optional<Date> winterBreakEnd;
    std::optional<Date> seasonEndDate;

public:
    static SeasonPlan build(int seasonYear, const LeagueRules& rules);
    void finalizeFromFixture(const League& league, const LeagueRules& rules);

    int getSeasonYear() const { return seasonYear; }
    const Date& getPreseasonStart() const { return preseasonStart; }
    const Date& getKickoff() const { return kickoff; }
    const Date& getNextPreseasonStart() const { return nextPreseasonStart; }
    const TransferWindow& getSummerWindow() const { return summerWindow; }
    const TransferWindow& getWinterWindow() const { return winterWindow; }
    const std::optional<Date>& getWinterBreakStart() const { return winterBreakStart; }
    const std::optional<Date>& getWinterBreakEnd() const { return winterBreakEnd; }
    const std::optional<Date>& getSeasonEndDate() const { return seasonEndDate; }
};
