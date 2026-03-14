#include "LeagueRules.h"

namespace {
    bool dateLess(const Date& lhs, const Date& rhs) {
        return lhs < rhs;
    }
}

bool TransferWindow::contains(const Date& date) const {
    return !dateLess(date, start) && dateLess(date, endExclusive);
}

Date LeagueRules::secondSaturdayOfAugust(int year) {
    Date d(year, Month::August, 1);
    while (d.getDayOfWeek() != 6) {
        d.advanceDay();
    }
    d.advanceDay();
    d.advanceDay();
    d.advanceDay();
    d.advanceDay();
    d.advanceDay();
    d.advanceDay();
    d.advanceDay();
    return d;
}

LeagueRules LeagueRules::makeSuperLig() {
    LeagueRules rules;
    rules.leagueId = "TR_SUPER_LIG";
    rules.leagueName = "Super Lig";
    rules.teamCount = 18;
    rules.matchdaysPerSeason = 2 * (rules.teamCount - 1);
    rules.firstHalfRounds = rules.teamCount - 1;

    rules.preseasonStart = [](int year) {
        return Date(year, Month::July, 1);
        };

    rules.kickoffDate = [](int year) {
        return LeagueRules::secondSaturdayOfAugust(year);
        };

    rules.nextPreseasonStart = [](int year) {
        return Date(year + 1, Month::July, 1);
        };

    rules.summerWindow = [](int year) {
        return TransferWindow{ Date(year, Month::July, 1), Date(year, Month::September, 1) };
        };

    rules.winterWindow = [](int year) {
        return TransferWindow{ Date(year + 1, Month::January, 1), Date(year + 1, Month::February, 1) };
        };

    rules.winterBreakEnabled = true;
    rules.winterBreakLengthDays = 28;
    rules.winterBreakAfterRoundIndex = 17;
    rules.matchSpacingDays = 7;
    rules.matchdayDistributionOffsets = { 0,0,1,1,1,1,2,2,2 };

    return rules;
}