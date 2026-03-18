#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Date.h"
#include "Game.h"
#include "League.h"
#include "LeagueRules.h"
#include "MatchScheduler.h"
#include "SeasonPlan.h"

namespace {

    std::string dayOfWeekToString(int dayOfWeek) {
        switch (dayOfWeek) {
        case 1: return "Mon";
        case 2: return "Tue";
        case 3: return "Wed";
        case 4: return "Thu";
        case 5: return "Fri";
        case 6: return "Sat";
        case 7: return "Sun";
        default: return "?";
        }
    }

    std::string monthToTwoDigits(Month month) {
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(month);
        return oss.str();
    }

    std::string dateToString(const Date& date) {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << date.getYear() << "-"
            << monthToTwoDigits(date.getMonth()) << "-"
            << std::setw(2) << std::setfill('0') << date.getDay()
            << " (" << dayOfWeekToString(date.getDayOfWeek()) << ")";
        return oss.str();
    }

    bool dateEquals(const Date& lhs, const Date& rhs) {
        return !(lhs < rhs) && !(rhs < lhs);
    }


    bool dateGreaterOrEqual(const Date& lhs, const Date& rhs) {
        return !(lhs < rhs);
    }

    std::string stateToString(GameState state) {
        switch (state) {
        case GameState::PreSeason: return "PreSeason";
        case GameState::InSeason: return "InSeason";
        case GameState::PostSeason: return "PostSeason";
        default: return "Unknown";
        }
    }

    void assertOrThrow(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    std::string optionalDateToString(const std::optional<Date>& date) {
        if (!date.has_value()) {
            return "n/a";
        }
        return dateToString(*date);
    }

}
void validateSortedStandings(const League& league) {
    const auto sorted = league.getSortedStandings();
    for (std::size_t i = 1; i < sorted.size(); ++i) {
        const StandingsEntry& prev = sorted[i - 1];
        const StandingsEntry& curr = sorted[i];

        const bool correctlyOrdered =
            (prev.points > curr.points) ||
            (prev.points == curr.points && prev.goalDifference > curr.goalDifference) ||
            (prev.points == curr.points && prev.goalDifference == curr.goalDifference && prev.goalsFor > curr.goalsFor) ||
            (prev.points == curr.points && prev.goalDifference == curr.goalDifference && prev.goalsFor == curr.goalsFor && prev.teamId < curr.teamId) ||
            (prev.points == curr.points && prev.goalDifference == curr.goalDifference && prev.goalsFor == curr.goalsFor && prev.teamId == curr.teamId);

        assertOrThrow(correctlyOrdered, "Sorted standings order is invalid.");
    }
}

void validateSeasonOutcome(Game& game, const LeagueRules& rules) {
    League& league = game.getLeague();

    int playedFixtures = 0;
    int totalPlayedFromStandings = 0;
    int totalGoalsFor = 0;
    int totalGoalsAgainst = 0;
    int totalWins = 0;
    int totalLosses = 0;

    for (const auto& [date, matches] : league.getFixture()) {
        (void)date;
        for (const auto& match : matches) {
            assertOrThrow(!(match.played && match.eventEnqueued),
                "Illegal fixture state: played and eventEnqueued cannot both be true.");
            if (match.played) {
                ++playedFixtures;
                assertOrThrow(match.homeGoals >= 0 && match.awayGoals >= 0,
                    "Played fixture must contain finalized score.");
            }
            else {
                assertOrThrow(match.homeGoals == -1 && match.awayGoals == -1,
                    "Unplayed fixture must keep score fields at -1.");
            }
        }
    }

    for (const auto& [teamId, entry] : league.getStandings()) {
        (void)teamId;
        assertOrThrow(league.findTeamById(entry.teamId) != nullptr,
            "Standings lookup by TeamId failed.");
        assertOrThrow(!league.getTeamName(entry.teamId).empty(),
            "Team name display lookup failed.");

        totalPlayedFromStandings += entry.played;
        totalGoalsFor += entry.goalsFor;
        totalGoalsAgainst += entry.goalsAgainst;
        totalWins += entry.wins;
        totalLosses += entry.losses;
    }

    assertOrThrow(playedFixtures == rules.teamCount * (rules.teamCount - 1),
        "Played fixture count mismatch at season completion.");
    assertOrThrow(totalPlayedFromStandings == playedFixtures * 2,
        "Standings played total mismatch at season completion.");
    assertOrThrow(totalGoalsFor == totalGoalsAgainst,
        "Standings goalsFor/goalsAgainst invariant failed.");
    assertOrThrow(totalWins == totalLosses,
        "Standings wins/losses invariant failed.");

    validateSortedStandings(league);

    bool duplicateResolveBlocked = false;
    for (const auto& [date, matches] : league.getFixture()) {
        for (const auto& match : matches) {
            if (!match.played) {
                continue;
            }
            try {
                league.applyMatchResult(date, match.homeId, match.awayId, MatchResult{ match.homeGoals, match.awayGoals });
            }
            catch (const std::logic_error&) {
                duplicateResolveBlocked = true;
            }
            assertOrThrow(duplicateResolveBlocked, "Duplicate match resolution should be blocked.");
            return;
        }
    }

    throw std::runtime_error("No played fixture found to validate duplicate resolution guard.");
}

int main() {
    try {
        Game game;

        const int simulationDays = 1500;
        const int tickEvery = 50;
        const bool failFast = true;

        int seasonsObserved = 0;
        int lastSeasonYearLogged = game.getDate().getYear() - 1;
        int lastSeasonStartObservedYear = game.getDate().getYear() - 1;
        int lastSeasonMatchEvents = 0;
        std::optional<Date> lastWeeklyLogDate;
        std::optional<std::string> lastNewMonthLogKey;
        
        const LeagueRules& rules = game.getRules();
        const SeasonPlan& bootPlan = game.getSeasonPlan();
        const League& bootLeague = game.getLeague();

        bool lastAllMatchesPlayed = bootLeague.allMatchesPlayed();
        bool seasonOutcomeValidated = false;
        bool prevTransferOpen = bootPlan.getSummerWindow().contains(game.getDate()) || bootPlan.getWinterWindow().contains(game.getDate());
        bool wasSummerOpen = bootPlan.getSummerWindow().contains(game.getDate());
        bool wasWinterOpen = bootPlan.getWinterWindow().contains(game.getDate());

        const int expectedTotalMatches =
            (rules.teamCount * (rules.teamCount - 1));

        std::cout << "[Boot] year=" << game.getDate().getYear()
            << " date=" << dateToString(game.getDate())
            << " teams=" << bootLeague.getTeams().size()
            << " league=" << rules.leagueName
            << " preseasonStart=" << dateToString(bootPlan.getPreseasonStart())
            << " kickoff=" << dateToString(bootPlan.getKickoff())
            << " summerWindow=[" << dateToString(bootPlan.getSummerWindow().start)
            << ", " << dateToString(bootPlan.getSummerWindow().endExclusive) << ")"
            << " winterWindow=[" << dateToString(bootPlan.getWinterWindow().start)
            << ", " << dateToString(bootPlan.getWinterWindow().endExclusive) << ")"
            << "\n";

        for (int simDay = 1; simDay <= simulationDays; ++simDay) {
            game.updateDaily();

            const Date& date = game.getDate();
            const SeasonPlan& plan = game.getSeasonPlan();
            const League& league = game.getLeague();
            const TransferWindow& summer = plan.getSummerWindow();
            const TransferWindow& winter = plan.getWinterWindow();

            if (simDay % tickEvery == 0) {
                std::cout << "[Tick] simDay=" << simDay
                    << " date=" << dateToString(date)
                    << " state=" << stateToString(game.getState())
                    << " transferOpen=" << (game.getTransferRoom().isOpen() ? "true" : "false")
                    << " matchEvents=" << game.getMatchScheduler().debugGeneratedMatchEvents()
                    << "\n";
            }

            const std::string monthLogKey = std::to_string(date.getYear()) + "-" +
                std::to_string(static_cast<int>(date.getMonth()));
            if (date.isNewMonth() && (!lastNewMonthLogKey.has_value() || *lastNewMonthLogKey != monthLogKey)) {
                std::cout << "[NewMonth] date=" << dateToString(date)
                    << " state=" << stateToString(game.getState())
                    << " summerOpen=" << (summer.contains(date) ? "true" : "false")
                    << " winterOpen=" << (winter.contains(date) ? "true" : "false")
                    << " transferOpen=" << (game.getTransferRoom().isOpen() ? "true" : "false")
                    << "\n";
                lastNewMonthLogKey = monthLogKey;
            }

            if (date.getDayOfWeek() == 1 && (!lastWeeklyLogDate.has_value() || !dateEquals(*lastWeeklyLogDate, date))) {
                std::cout << "[Weekly] date=" << dateToString(date)
                    << " state=" << stateToString(game.getState())
                    << " allMatchesPlayed=" << (league.allMatchesPlayed() ? "true" : "false")
                    << "\n";
                lastWeeklyLogDate = date;
            }

            const bool summerOpenNow = summer.contains(date);
            const bool winterOpenNow = winter.contains(date);

          
            if (!wasSummerOpen && summerOpenNow) {
                std::cout << "[TransferWindowOpen] type=Summer date=" << dateToString(date) << "\n";
            }
            if (wasSummerOpen && !summerOpenNow) {
                const bool closeBoundaryIsClosed = !summer.contains(summer.endExclusive);
                assertOrThrow(!failFast || closeBoundaryIsClosed,
                    "Summer transfer window must be closed on endExclusive boundary at " +
                    dateToString(summer.endExclusive));
                std::cout << "[TransferWindowClose] type=Summer date=" << dateToString(date)
                    << " closeBoundary=" << dateToString(summer.endExclusive)
                    << "\n";
            }

          
            if (!wasWinterOpen && winterOpenNow) {
                std::cout << "[TransferWindowOpen] type=Winter date=" << dateToString(date) << "\n";
            }
            if (wasWinterOpen && !winterOpenNow) {
                const bool closeBoundaryIsClosed = !winter.contains(winter.endExclusive);
                assertOrThrow(!failFast || closeBoundaryIsClosed,
                    "Winter transfer window must be closed on endExclusive boundary at " +
                    dateToString(winter.endExclusive));
                std::cout << "[TransferWindowClose] type=Winter date=" << dateToString(date)
                    << " closeBoundary=" << dateToString(winter.endExclusive)
                    << "\n";
            }

            
            wasSummerOpen = summerOpenNow;
            wasWinterOpen = winterOpenNow;

            if (date.getMonth() == Month::July && date.getDay() == 1 && date.getYear() != lastSeasonYearLogged) {
                lastSeasonYearLogged = date.getYear();
                ++seasonsObserved;
            }

            if (dateEquals(date, plan.getPreseasonStart()) && date.getYear() != lastSeasonStartObservedYear) {
                lastSeasonStartObservedYear = date.getYear();

                const int teams = static_cast<int>(league.getTeams().size());
                const bool fixtureGenerated = league.isSeasonFixtureGenerated();
                const int fixtureDays = league.debugFixtureDayCount();
                const int totalMatches = league.debugTotalFixtureMatches();
                const int totalGeneratedMatchEvents = game.getMatchScheduler().debugGeneratedMatchEvents();
                const int generatedThisSeason = totalGeneratedMatchEvents - lastSeasonMatchEvents;
                lastSeasonMatchEvents = totalGeneratedMatchEvents;

                const std::optional<Date> firstHalfEnd = league.tryGetMatchdayEndDate(rules.firstHalfRounds);
                const std::optional<Date> finalRoundEnd = league.tryGetMatchdayEndDate(rules.matchdaysPerSeason);

                std::cout << "[SeasonStart] year=" << plan.getSeasonYear()
                    << " date=" << dateToString(date)
                    << " teams=" << teams
                    << " fixtureGenerated=" << (fixtureGenerated ? "true" : "false")
                    << " fixtureDays=" << fixtureDays
                    << " totalMatches=" << totalMatches
                    << " matchEventsThisSeason=" << generatedThisSeason
                    << " firstHalfEnd=" << optionalDateToString(firstHalfEnd)
                    << " finalRoundEnd=" << optionalDateToString(finalRoundEnd)
                    << "\n";

                assertOrThrow(!failFast || teams == rules.teamCount,
                    "SeasonStart validation failed at " + dateToString(date) +
                    ": team count mismatch. expected=" + std::to_string(rules.teamCount) +
                    " actual=" + std::to_string(teams));
                assertOrThrow(!failFast || fixtureGenerated,
                    "SeasonStart validation failed at " + dateToString(date) +
                    ": fixture must be generated.");
                assertOrThrow(!failFast || totalMatches == expectedTotalMatches,
                    "SeasonStart validation failed at " + dateToString(date) +
                    ": total fixture matches mismatch. expected=" + std::to_string(expectedTotalMatches) +
                    " actual=" + std::to_string(totalMatches));
                assertOrThrow(!failFast || firstHalfEnd.has_value(),
                    "SeasonStart validation failed at " + dateToString(date) +
                    ": first-half matchday end date missing (index=" + std::to_string(rules.firstHalfRounds) + ").");
                assertOrThrow(!failFast || finalRoundEnd.has_value(),
                    "SeasonStart validation failed at " + dateToString(date) +
                    ": last-round matchday end date missing (index=" + std::to_string(rules.matchdaysPerSeason) + ").");
                assertOrThrow(!failFast || plan.getSeasonEndDate().has_value(),
                    "SeasonStart validation failed at " + dateToString(date) +
                    ": seasonEndDate should be finalized.");

                if (rules.winterBreakEnabled) {
                    assertOrThrow(!failFast || plan.getWinterBreakStart().has_value(),
                        "SeasonStart validation failed at " + dateToString(date) +
                        ": winterBreakStart missing while winter break enabled.");
                    assertOrThrow(!failFast || plan.getWinterBreakEnd().has_value(),
                        "SeasonStart validation failed at " + dateToString(date) +
                        ": winterBreakEnd missing while winter break enabled.");
                    std::cout << "[WinterBreak] start=" << optionalDateToString(plan.getWinterBreakStart())
                        << " end=" << optionalDateToString(plan.getWinterBreakEnd()) << "\n";
                }
            }

            if (dateEquals(date, plan.getKickoff())) {
                std::cout << "[Kickoff] date=" << dateToString(date)
                    << " seasonYear=" << plan.getSeasonYear()
                    << "\n";
            }

            const bool allMatchesPlayedNow = league.allMatchesPlayed();
            if (!lastAllMatchesPlayed && allMatchesPlayedNow) {
                validateSeasonOutcome(game, rules);
                seasonOutcomeValidated = true;
                const std::optional<Date> seasonEnd = plan.getSeasonEndDate();
                assertOrThrow(!failFast || seasonEnd.has_value(),
                    "SeasonComplete validation failed at " + dateToString(date) +
                    ": seasonEndDate should exist once all matches are played.");

                if (seasonEnd.has_value()) {
                    assertOrThrow(!failFast || dateGreaterOrEqual(date, *seasonEnd),
                        "SeasonComplete validation failed at " + dateToString(date) +
                        ": completion date is before seasonEndDate=" + dateToString(*seasonEnd));
                }

                std::cout << "[SeasonComplete] date=" << dateToString(date)
                    << " seasonYear=" << plan.getSeasonYear()
                    << " totalMatches=" << league.debugTotalFixtureMatches()
                    << " seasonEnd=" << optionalDateToString(seasonEnd)
                    << " state=" << stateToString(game.getState())
                    << "\n";

                if (seasonEnd.has_value() && dateGreaterOrEqual(date, *seasonEnd)) {
                    std::cout << "[PostSeasonCheck] date=" << dateToString(date)
                        << " state=" << stateToString(game.getState())
                        << "\n";
                }
            }
            lastAllMatchesPlayed = allMatchesPlayedNow;
        }

        if (rules.winterBreakEnabled) {
            const SeasonPlan& plan = game.getSeasonPlan();
            const League& league = game.getLeague();
            const auto firstHalfEnd = league.tryGetMatchdayEndDate(17);
            const auto fullSeasonEnd = league.tryGetMatchdayEndDate(34);
            assertOrThrow(!failFast || firstHalfEnd.has_value(),
                "Final validation failed: matchdayEndDate(17) missing with winter break enabled.");
            assertOrThrow(!failFast || fullSeasonEnd.has_value(),
                "Final validation failed: matchdayEndDate(34) missing with winter break enabled.");
            (void)plan;
        }
        assertOrThrow(!failFast || seasonOutcomeValidated, "Final validation failed: no completed season outcome was validated.");

        std::cout << "[Done] seasonsObserved=" << seasonsObserved
            << " totalGeneratedMatchEvents=" << game.getMatchScheduler().debugGeneratedMatchEvents()
            << " finalDate=" << dateToString(game.getDate())
            << "\n";

        return EXIT_SUCCESS;
    }
    catch (const std::exception& ex) {
        std::cerr << "[FAIL] " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}