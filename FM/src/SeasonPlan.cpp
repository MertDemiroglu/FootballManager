#include"SeasonPlan.h"
#include"fm/common/DateUtils.h"
#include"League.h"

SeasonPlan SeasonPlan::build(int targetSeasonYear, const LeagueRules& rules) {
    SeasonPlan plan;
    plan.seasonYear = targetSeasonYear;
    plan.preseasonStart = rules.preseasonStart(targetSeasonYear);
    plan.kickoff = rules.kickoffDate(targetSeasonYear);
    plan.nextPreseasonStart = rules.nextPreseasonStart(targetSeasonYear);
    plan.summerWindow = rules.summerWindow(targetSeasonYear);
    plan.winterWindow = rules.winterWindow(targetSeasonYear);

    plan.winterBreakStart.reset();
    plan.winterBreakEnd.reset();
    plan.seasonEndDate.reset();
    return plan;
}

void SeasonPlan::finalizeFromFixture(const League& league, const LeagueRules& rules) {
    if (league.getFixture().empty()) {
        winterBreakStart.reset();
        winterBreakEnd.reset();
        seasonEndDate.reset();
        return;
    }

    seasonEndDate = league.getLastFixtureDate();

    if (!rules.winterBreakEnabled) {
        winterBreakStart.reset();
        winterBreakEnd.reset();
        return;
    }

    const auto anchor = league.tryGetMatchWeekEndDate(rules.winterBreakAfterRoundIndex);
    if (!anchor.has_value()) {
        winterBreakStart.reset();
        winterBreakEnd.reset();
        return;
    }

    winterBreakStart = DateUtils::addDays(*anchor, 1);
    winterBreakEnd = DateUtils::addDays(*winterBreakStart, rules.winterBreakLengthDays);
}
