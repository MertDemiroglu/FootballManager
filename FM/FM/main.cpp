#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

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
struct TeamSeasonStatsTotals {
    int totalPlayed = 0;
    int totalGoalsFor = 0;
    int totalGoalsAgainst = 0;
    int totalWins = 0;
    int totalLosses = 0;
    int totalPoints = 0;
};

TeamSeasonStatsTotals computeTeamSeasonStatsTotals(const std::unordered_map<TeamId, TeamSeasonStats>& statsByTeam) {
    TeamSeasonStatsTotals totals;
    for (const auto& [teamId, entry] : statsByTeam) {
        (void)teamId;
        totals.totalPlayed += entry.played;
        totals.totalGoalsFor += entry.goalsFor;
        totals.totalGoalsAgainst += entry.goalsAgainst;
        totals.totalWins += entry.wins;
        totals.totalLosses += entry.losses;
        totals.totalPoints += entry.points;
    }
    return totals;
}

void validateCurrentPlayerSeasonStats(const League& league, int expectedSeasonYear) {
    for (const auto& [teamName, team] : league.getTeams()) {
        (void)teamName;
        for (const auto& player : team->getPlayers()) {
            const PlayerSeasonStats& stats = player->getCurrentSeasonStats();
            assertOrThrow(stats.playerId == player->getId(),
                "Player currentSeasonStats.playerId must match player id.");
            assertOrThrow(stats.seasonYear == expectedSeasonYear,
                "Player currentSeasonStats.seasonYear must match current season year.");
        }
    }
}

void validateSeasonStatsAtCompletion(const League& league, const LeagueRules& rules) {
    const auto& teamStatsByTeam = league.getCurrentTeamSeasonStats();
    const auto& standings = league.getStandings();
    const int teamCount = static_cast<int>(league.getTeams().size());
    const int expectedPlayedPerTeam = rules.matchdaysPerSeason;

    assertOrThrow(static_cast<int>(teamStatsByTeam.size()) == teamCount,
        "Team season stats entry count must match team count.");

    int totalStandingsPoints = 0;
    for (const auto& [teamId, entry] : standings) {
        (void)teamId;
        totalStandingsPoints += entry.points;
    }

    for (const auto& [teamId, stats] : teamStatsByTeam) {
        assertOrThrow(league.getCurrentTeamSeasonStatsFor(teamId) != nullptr,
            "Current team season stats lookup must not return null.");
        assertOrThrow(stats.teamId == teamId,
            "Team season stats teamId must match map key.");
        assertOrThrow(stats.seasonYear == league.getCurrentSeasonYear(),
            "Team season stats seasonYear must match current season year.");
        assertOrThrow(stats.played == expectedPlayedPerTeam,
            "Each team must have the expected played count in team season stats.");

        const auto standingsIt = standings.find(teamId);
        assertOrThrow(standingsIt != standings.end(),
            "Standings entry must exist for team season stats team.");
        const StandingsEntry& standingsEntry = standingsIt->second;
        assertOrThrow(stats.played == standingsEntry.played,
            "Team season stats played must match standings.");
        assertOrThrow(stats.wins == standingsEntry.wins,
            "Team season stats wins must match standings.");
        assertOrThrow(stats.draws == standingsEntry.draws,
            "Team season stats draws must match standings.");
        assertOrThrow(stats.losses == standingsEntry.losses,
            "Team season stats losses must match standings.");
        assertOrThrow(stats.goalsFor == standingsEntry.goalsFor,
            "Team season stats goalsFor must match standings.");
        assertOrThrow(stats.goalsAgainst == standingsEntry.goalsAgainst,
            "Team season stats goalsAgainst must match standings.");
        assertOrThrow(stats.points == standingsEntry.points,
            "Team season stats points must match standings.");
    }

    const TeamSeasonStatsTotals totals = computeTeamSeasonStatsTotals(teamStatsByTeam);
    assertOrThrow(totals.totalPlayed == rules.teamCount * expectedPlayedPerTeam,
        "Total team season stats played mismatch.");
    assertOrThrow(totals.totalGoalsFor == totals.totalGoalsAgainst,
        "Team season stats goalsFor/goalsAgainst invariant failed.");
    assertOrThrow(totals.totalWins == totals.totalLosses,
        "Team season stats wins/losses invariant failed.");
    assertOrThrow(totals.totalPoints == totalStandingsPoints,
        "Team season stats total points must match standings total points.");
}

void validateSeasonRolloverState(const League& league, int archivedSeasonYear, const LeagueRules& rules) {
    const auto& archivedBySeason = league.getArchivedTeamSeasonStatsBySeason();
    const auto archivedSeasonIt = archivedBySeason.find(archivedSeasonYear);
    assertOrThrow(archivedSeasonIt != archivedBySeason.end(),
        "Archived team season stats must contain the completed season.");
    assertOrThrow(static_cast<int>(archivedSeasonIt->second.size()) == rules.teamCount,
        "Archived team season stats entry count must match team count.");

    for (const auto& [teamName, team] : league.getTeams()) {
        (void)teamName;
        const TeamId teamId = team->getId();
        const TeamSeasonStats* archivedStats = league.getArchivedTeamSeasonStatsFor(teamId, archivedSeasonYear);
        assertOrThrow(archivedStats != nullptr,
            "Archived team season stats lookup must not return null.");
        assertOrThrow(archivedStats->seasonYear == archivedSeasonYear,
            "Archived team season stats seasonYear must match archived season.");

        const TeamSeasonStats* currentStats = league.getCurrentTeamSeasonStatsFor(teamId);
        assertOrThrow(currentStats != nullptr,
            "Current team season stats lookup after rollover must not return null.");
        assertOrThrow(currentStats->seasonYear == league.getCurrentSeasonYear(),
            "Current team season stats seasonYear must match the new season year.");
        assertOrThrow(currentStats->played == 0 && currentStats->points == 0 && currentStats->goalsFor == 0 && currentStats->goalsAgainst == 0,
            "Current team season stats must be reset for the new season.");
    }

    assertOrThrow(static_cast<int>(league.getCurrentTeamSeasonStats().size()) == static_cast<int>(league.getTeams().size()),
        "Current team season stats entry count after rollover must match team count.");

    validateCurrentPlayerSeasonStats(league, league.getCurrentSeasonYear());

    for (const auto& [teamName, team] : league.getTeams()) {
        (void)teamName;
        for (const auto& player : team->getPlayers()) {
            const auto& archivedStatsByYear = player->getArchivedSeasonStatsByYear();
            const auto archivedPlayerIt = archivedStatsByYear.find(archivedSeasonYear);
            assertOrThrow(archivedPlayerIt != archivedStatsByYear.end(),
                "Archived player season stats must contain the completed season.");
            assertOrThrow(archivedPlayerIt->second.playerId == player->getId(),
                "Archived player season stats playerId must match player id.");
            assertOrThrow(archivedPlayerIt->second.seasonYear == archivedSeasonYear,
                "Archived player season stats seasonYear must match archived season.");

            const PlayerSeasonStats& currentStats = player->getCurrentSeasonStats();
            assertOrThrow(currentStats.playerId == player->getId(),
                "Reset player season stats playerId must remain stable.");
            assertOrThrow(currentStats.seasonYear == league.getCurrentSeasonYear(),
                "Reset player season stats seasonYear must match new season year.");
            assertOrThrow(currentStats.appearances == 0 && currentStats.starts == 0 && currentStats.substituteAppearances == 0 &&
                currentStats.minutesPlayed == 0 && currentStats.goals == 0 && currentStats.assists == 0 &&
                currentStats.yellowCards == 0 && currentStats.redCards == 0 && currentStats.cleanSheets == 0 &&
                currentStats.goalsConcededWhileOnPitch == 0,
                "Reset player season stats must start from zero values.");
        }
    }
}

void validateMatchHistoryIntegrity(const League& league, const LeagueRules& rules) {
    const auto& history = league.getCurrentSeasonHistory();
    const int expectedTotalMatches = rules.teamCount * (rules.teamCount - 1);
    assertOrThrow(static_cast<int>(history.size()) == expectedTotalMatches,
        "Current season history size mismatch at season completion.");

    std::unordered_set<std::string> dedupe;
    for (const MatchRecord& record : history) {
        assertOrThrow(record.seasonYear == league.getCurrentSeasonYear(),
            "MatchRecord seasonYear does not match current league season.");
        assertOrThrow(league.hasTeam(record.homeId),
            "History record contains invalid home TeamId.");
        assertOrThrow(league.hasTeam(record.awayId),
            "History record contains invalid away TeamId.");
        assertOrThrow(record.homeId != record.awayId,
            "History record cannot contain same team twice.");
        assertOrThrow(record.homeGoals >= 0 && record.awayGoals >= 0,
            "History record cannot contain negative goals.");
        const std::string key = std::to_string(record.seasonYear) + ":" +
            std::to_string(record.date.getYear()) + "-" +
            std::to_string(static_cast<int>(record.date.getMonth())) + "-" +
            std::to_string(record.date.getDay()) + ":" +
            std::to_string(record.homeId) + ":" + std::to_string(record.awayId);
        assertOrThrow(dedupe.insert(key).second,
            "Duplicate history record detected for a played match.");
    }
}

void validateTeamHistoryQueries(const League& league, const LeagueRules& rules) {
    const int expectedMatchesPerTeam = (rules.teamCount - 1) * 2;
    bool checkedAnyTeam = false;

    for (const auto& [teamName, team] : league.getTeams()) {
        (void)teamName;
        const TeamId teamId = team->getId();
        const auto currentSeasonMatches = league.getMatchesForTeamInCurrentSeason(teamId);
        const auto seasonMatches = league.getMatchesForTeamInSeason(teamId, league.getCurrentSeasonYear());

        assertOrThrow(!currentSeasonMatches.empty(),
            "Team current-season history query unexpectedly returned empty.");
        assertOrThrow(static_cast<int>(currentSeasonMatches.size()) == expectedMatchesPerTeam,
            "Team current-season history query count mismatch.");
        assertOrThrow(currentSeasonMatches.size() == seasonMatches.size(),
            "Season-scoped history query should match current-season query for active season.");

        const std::string recentForm = league.getRecentFormString(teamId, 5);
        assertOrThrow(recentForm.size() <= 5,
            "Recent form string cannot exceed requested size.");
        for (char c : recentForm) {
            assertOrThrow(c == 'W' || c == 'D' || c == 'L',
                "Recent form string contains invalid characters.");
        }

        const auto recentMatches = league.getLastMatchesForTeam(teamId, 5);
        assertOrThrow(recentMatches.size() <= 5,
            "Last-matches query cannot exceed requested size.");

        checkedAnyTeam = true;
    }

    assertOrThrow(checkedAnyTeam, "No team found while validating history queries.");
}

void validateArchiveState(const League& league, int archivedSeasonYear, const LeagueRules& rules) {
    const auto& archived = league.getArchivedHistoryBySeason();
    const auto it = archived.find(archivedSeasonYear);
    assertOrThrow(it != archived.end(),
        "Archived history entry missing after season rollover.");

    const int expectedTotalMatches = rules.teamCount * (rules.teamCount - 1);
    assertOrThrow(static_cast<int>(it->second.size()) == expectedTotalMatches,
        "Archived season history size mismatch after rollover.");
    assertOrThrow(league.getCurrentSeasonHistory().empty(),
        "New season current history must start empty after rollover.");
}
struct StandingsTotals {
    int totalPlayed = 0;
    int totalGoalsFor = 0;
    int totalGoalsAgainst = 0;
    int totalWins = 0;
    int totalLosses = 0;
};

StandingsTotals computeStandingsTotals(const std::vector<StandingsEntry>& sorted) {
    StandingsTotals totals;
    for (const auto& entry : sorted) {
        totals.totalPlayed += entry.played;
        totals.totalGoalsFor += entry.goalsFor;
        totals.totalGoalsAgainst += entry.goalsAgainst;
        totals.totalWins += entry.wins;
        totals.totalLosses += entry.losses;
    }
    return totals;
}

void printSeasonStandings(const League& league, int seasonYear) {
    const auto sorted = league.getSortedStandings();
    const int teamCount = static_cast<int>(league.getTeams().size());

    assertOrThrow(!sorted.empty(),
        "Standings print failed: sorted standings cannot be empty.");
    assertOrThrow(static_cast<int>(sorted.size()) == teamCount,
        "Standings print failed: standings entry count must match team count.");

    std::cout << "[Standings] seasonYear=" << seasonYear << "\n";
    std::cout << "#  "
        << std::left << std::setw(20) << "Team"
        << std::right << std::setw(4) << "P"
        << std::setw(4) << "W"
        << std::setw(4) << "D"
        << std::setw(4) << "L"
        << std::setw(5) << "GF"
        << std::setw(5) << "GA"
        << std::setw(5) << "GD"
        << std::setw(5) << "Pts"
        << "\n";

    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const StandingsEntry& entry = sorted[i];
        const std::string& teamName = league.getTeamName(entry.teamId);
        assertOrThrow(!teamName.empty(),
            "Standings print failed: team name lookup cannot be empty.");

        std::cout << std::setw(2) << (i + 1) << ". "
            << std::left << std::setw(20) << teamName
            << std::right << std::setw(4) << entry.played
            << std::setw(4) << entry.wins
            << std::setw(4) << entry.draws
            << std::setw(4) << entry.losses
            << std::setw(5) << entry.goalsFor
            << std::setw(5) << entry.goalsAgainst
            << std::setw(5) << entry.goalDifference
            << std::setw(5) << entry.points
            << "\n";
    }
}

void printStandingsSummary(const League& league, int seasonYear) {
    const auto sorted = league.getSortedStandings();
    const int teamCount = static_cast<int>(league.getTeams().size());

    assertOrThrow(!sorted.empty(),
        "Standings summary failed: sorted standings cannot be empty.");
    assertOrThrow(static_cast<int>(sorted.size()) == teamCount,
        "Standings summary failed: standings entry count must match team count.");

    const std::string& champion = league.getTeamName(sorted.front().teamId);
    const std::string& bottom = league.getTeamName(sorted.back().teamId);
    assertOrThrow(!champion.empty(),
        "Standings summary failed: champion team name cannot be empty.");
    assertOrThrow(!bottom.empty(),
        "Standings summary failed: bottom team name cannot be empty.");

    const StandingsTotals totals = computeStandingsTotals(sorted);

    std::cout << "[StandingsSummary] seasonYear=" << seasonYear
        << " champion=" << champion
        << " bottom=" << bottom
        << " teams=" << sorted.size()
        << " totalPlayed=" << totals.totalPlayed
        << " totalGF=" << totals.totalGoalsFor
        << " totalGA=" << totals.totalGoalsAgainst
        << " totalWins=" << totals.totalWins
        << " totalLosses=" << totals.totalLosses
        << "\n";
}

void maybeLogWeekly(const Date& date,
    const Game& game,
    const League& league,
    std::optional<Date>& lastWeeklyLogDate) {
    if (date.getDayOfWeek() != 1 || (lastWeeklyLogDate.has_value() && dateEquals(*lastWeeklyLogDate, date))) {
        return;
    }

    std::cout << "[Weekly] date=" << dateToString(date)
        << " state=" << stateToString(game.getState())
        << " allMatchesPlayed=" << (league.allMatchesPlayed() ? "true" : "false")
        << "\n";
    lastWeeklyLogDate = date;
}

void maybeLogMonthBoundary(const Date& date,
    const Game& game,
    const TransferWindow& summer,
    const TransferWindow& winter,
    std::optional<std::string>& lastNewMonthLogKey) {
    const std::string monthLogKey = std::to_string(date.getYear()) + "-" +
        std::to_string(static_cast<int>(date.getMonth()));
    if (!date.isNewMonth() || (lastNewMonthLogKey.has_value() && *lastNewMonthLogKey == monthLogKey)) {
        return;
    }

    std::cout << "[NewMonth] date=" << dateToString(date)
        << " state=" << stateToString(game.getState())
        << " summerOpen=" << (summer.contains(date) ? "true" : "false")
        << " winterOpen=" << (winter.contains(date) ? "true" : "false")
        << " transferOpen=" << (game.getTransferRoom().isOpen() ? "true" : "false")
        << "\n";
    lastNewMonthLogKey = monthLogKey;
}

void maybeLogTransferWindowTransitions(const Date& preDate,
    const Date& postDate,
    const TransferWindow& preSummer,
    const TransferWindow& postSummer,
    const TransferWindow& preWinter,
    const TransferWindow& postWinter,
    bool failFast) {
    const bool summerOpenBefore = preSummer.contains(preDate);
    const bool summerOpenNow = postSummer.contains(postDate);
    const bool winterOpenBefore = preWinter.contains(preDate);
    const bool winterOpenNow = postWinter.contains(postDate);

    if (!summerOpenBefore && summerOpenNow) {
        std::cout << "[TransferWindowOpen] type=Summer date=" << dateToString(postSummer.start)
            << " observedOn=" << dateToString(postDate)
            << " previousSnapshot=" << dateToString(preDate)
            << "\n";
    }
    if (summerOpenBefore && !summerOpenNow) {
        const bool closeBoundaryIsClosed = !preSummer.contains(preSummer.endExclusive);
        assertOrThrow(!failFast || closeBoundaryIsClosed,
            "Summer transfer window must be closed on endExclusive boundary at " +
            dateToString(preSummer.endExclusive));
        std::cout << "[TransferWindowClose] type=Summer date=" << dateToString(preSummer.endExclusive)
            << " observedOn=" << dateToString(postDate)
            << " previousSnapshot=" << dateToString(preDate)
            << "\n";
    }

    if (!winterOpenBefore && winterOpenNow) {
        std::cout << "[TransferWindowOpen] type=Winter date=" << dateToString(postWinter.start)
            << " observedOn=" << dateToString(postDate)
            << " previousSnapshot=" << dateToString(preDate)
            << "\n";
    }
    if (winterOpenBefore && !winterOpenNow) {
        const bool closeBoundaryIsClosed = !preWinter.contains(preWinter.endExclusive);
        assertOrThrow(!failFast || closeBoundaryIsClosed,
            "Winter transfer window must be closed on endExclusive boundary at " +
            dateToString(preWinter.endExclusive));
        std::cout << "[TransferWindowClose] type=Winter date=" << dateToString(preWinter.endExclusive)
            << " observedOn=" << dateToString(postDate)
            << " previousSnapshot=" << dateToString(preDate)
            << "\n";
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

void validateUpcomingMatchQueries(const League& league) {
    bool validatedAnyTeam = false;
    for (const auto& [teamName, team] : league.getTeams()) {
        (void)teamName;
        const TeamId teamId = team->getId();
        const auto nextMatch = league.getNextMatchForTeam(teamId);
        const auto upcomingMatches = league.getUpcomingMatchesForTeam(teamId, 3);

        assertOrThrow(nextMatch.has_value(), "Each team should have a next-match preview when fixture exists.");
        assertOrThrow(!upcomingMatches.empty(), "Upcoming match preview query should not be empty.");
        assertOrThrow(upcomingMatches.front().homeId == nextMatch->homeId &&
            upcomingMatches.front().awayId == nextMatch->awayId &&
            upcomingMatches.front().matchweek == nextMatch->matchweek,
            "Next match preview must match the first upcoming match preview.");
        assertOrThrow(upcomingMatches.front().homeId == teamId || upcomingMatches.front().awayId == teamId,
            "Upcoming match preview must belong to the requested team.");
        validatedAnyTeam = true;
    }

    assertOrThrow(validatedAnyTeam, "No team found while validating upcoming match previews.");
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
    validateMatchHistoryIntegrity(league, rules);
    validateTeamHistoryQueries(league, rules);

    bool duplicateResolveBlocked = false;
    for (const auto& [date, matches] : league.getFixture()) {
        for (const auto& match : matches) {
            if (!match.played) {
                continue;
            }
            const std::size_t historySizeBeforeDuplicateAttempt = league.getCurrentSeasonHistory().size();
            try {
                league.applyMatchResult(date, match.homeId, match.awayId, MatchResult{ match.homeGoals, match.awayGoals });
            }
            catch (const std::logic_error&) {
                duplicateResolveBlocked = true;
            }
            assertOrThrow(duplicateResolveBlocked, "Duplicate match resolution should be blocked.");
            assertOrThrow(historySizeBeforeDuplicateAttempt == league.getCurrentSeasonHistory().size(),
                "Duplicate match resolution attempt should not append extra history.");
            return;
        }
    }

    throw std::runtime_error("No played fixture found to validate duplicate resolution guard.");
}

int main() {
    try {
        Game game;

        const int simulationDays = 500;
        const int tickEvery = 50;
        const bool failFast = true;

        int seasonsObserved = 0;
        int lastSeasonYearLogged = game.getDate().getYear() - 1;
        int lastSeasonStartObservedYear = game.getDate().getYear() - 1;
        int lastSeasonMatchEvents = 0;
        std::optional<Date> lastWeeklyLogDate;
        std::optional<std::string> lastNewMonthLogKey;
        std::optional<int> pendingArchiveValidationSeason;
        bool sawHistoryRefillAfterArchive = false;
        bool sawAnyHistoryRefillAfterArchive = false;
        
        const LeagueRules& rules = game.getRules();
        const SeasonPlan& bootPlan = game.getSeasonPlan();
        const League& bootLeague = game.getLeague();
        int lastObservedLeagueSeasonYear = bootLeague.getCurrentSeasonYear();

        bool lastAllMatchesPlayed = bootLeague.allMatchesPlayed();
        bool seasonOutcomeValidated = false;
        bool archiveValidated = false;
        int lastPrintedStandingsSeasonYear = bootPlan.getSeasonYear() - 1;
        int lastArchivedSeasonValidated = bootPlan.getSeasonYear() - 1;

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
            const Date preUpdateDate = game.getDate();
            const SeasonPlan preUpdatePlan = game.getSeasonPlan();
            game.updateDaily();

            const Date& date = game.getDate();
            const SeasonPlan& plan = game.getSeasonPlan();
            const League& league = game.getLeague();
            const TransferWindow& summer = plan.getSummerWindow();
            const TransferWindow& winter = plan.getWinterWindow();

            if (league.getCurrentSeasonYear() != lastObservedLeagueSeasonYear) {
                if (pendingArchiveValidationSeason.has_value()) {
                    validateArchiveState(league, *pendingArchiveValidationSeason, rules);
                    archiveValidated = true;
                    pendingArchiveValidationSeason.reset();
                    sawHistoryRefillAfterArchive = false;
                }
                lastObservedLeagueSeasonYear = league.getCurrentSeasonYear();
            }

            if (simDay % tickEvery == 0) {
                std::cout << "[Tick] simDay=" << simDay
                    << " date=" << dateToString(date)
                    << " state=" << stateToString(game.getState())
                    << " transferOpen=" << (game.getTransferRoom().isOpen() ? "true" : "false")
                    << " matchEvents=" << game.getMatchScheduler().debugGeneratedMatchEvents()
                    << "\n";
            }

            maybeLogMonthBoundary(date, game, summer, winter, lastNewMonthLogKey);
            maybeLogWeekly(date, game, league, lastWeeklyLogDate);
            maybeLogTransferWindowTransitions(preUpdateDate, date, preUpdatePlan.getSummerWindow(), summer,preUpdatePlan.getWinterWindow(), winter, failFast);

       
            if (date.getMonth() == Month::July && date.getDay() == 1 && date.getYear() != lastSeasonYearLogged) {
                lastSeasonYearLogged = date.getYear();
                ++seasonsObserved;
            }

            if (dateEquals(date, plan.getPreseasonStart()) && date.getYear() != lastSeasonStartObservedYear) {
                lastSeasonStartObservedYear = date.getYear();

                if (plan.getSeasonYear() > bootPlan.getSeasonYear() && lastArchivedSeasonValidated != plan.getSeasonYear() - 1) {
                    validateSeasonRolloverState(league, plan.getSeasonYear() - 1, rules);
                    lastArchivedSeasonValidated = plan.getSeasonYear() - 1;
                }

                const int teams = static_cast<int>(league.getTeams().size());
                const bool fixtureGenerated = league.isSeasonFixtureGenerated();
                const int fixtureDays = league.debugFixtureDayCount();
                const int totalMatches = league.debugTotalFixtureMatches();
                const int totalGeneratedMatchEvents = game.getMatchScheduler().debugGeneratedMatchEvents();
                const int generatedThisSeason = totalGeneratedMatchEvents - lastSeasonMatchEvents;
                lastSeasonMatchEvents = totalGeneratedMatchEvents;

                const std::optional<Date> firstHalfEnd = league.tryGetMatchWeekEndDate(rules.firstHalfRounds);
                const std::optional<Date> finalRoundEnd = league.tryGetMatchWeekEndDate(rules.matchdaysPerSeason);

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
                assertOrThrow(!failFast || league.getCurrentSeasonHistory().empty(),
                    "SeasonStart validation failed: current season history must be empty at preseason start.");
                assertOrThrow(!failFast || league.getCurrentSeasonYear() == plan.getSeasonYear(),
                    "SeasonStart validation failed: league current season year mismatch.");

                validateUpcomingMatchQueries(league);

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
            if (!sawHistoryRefillAfterArchive && archiveValidated && !league.getCurrentSeasonHistory().empty()) {
                sawHistoryRefillAfterArchive = true;
                sawAnyHistoryRefillAfterArchive = true;
            }
            if (dateEquals(date, plan.getKickoff())) {
                std::cout << "[Kickoff] date=" << dateToString(date)
                    << " seasonYear=" << plan.getSeasonYear()
                    << "\n";
            }

            const bool allMatchesPlayedNow = league.allMatchesPlayed();
            if (!lastAllMatchesPlayed && allMatchesPlayedNow) {
                validateSeasonOutcome(game, rules);
                validateSeasonStatsAtCompletion(league, rules);
                validateCurrentPlayerSeasonStats(league, league.getCurrentSeasonYear());
                seasonOutcomeValidated = true;
                pendingArchiveValidationSeason = plan.getSeasonYear();
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
                    << " historySize=" << league.getCurrentSeasonHistory().size()
                    << " seasonEnd=" << optionalDateToString(seasonEnd)
                    << " state=" << stateToString(game.getState())
                    << "\n";

                if (lastPrintedStandingsSeasonYear != plan.getSeasonYear()) {
                    printSeasonStandings(league, plan.getSeasonYear());
                    printStandingsSummary(league, plan.getSeasonYear());
                    lastPrintedStandingsSeasonYear = plan.getSeasonYear();
                }

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
            const auto firstHalfEnd = league.tryGetMatchWeekEndDate(17);
            const auto fullSeasonEnd = league.tryGetMatchWeekEndDate(34);
            assertOrThrow(!failFast || firstHalfEnd.has_value(),
                "Final validation failed: matchdayEndDate(17) missing with winter break enabled.");
            assertOrThrow(!failFast || fullSeasonEnd.has_value(),
                "Final validation failed: matchdayEndDate(34) missing with winter break enabled.");
            (void)plan;
        }
        assertOrThrow(!failFast || seasonOutcomeValidated, "Final validation failed: no completed season outcome was validated.");
        assertOrThrow(!failFast || archiveValidated, "Final validation failed: no archived season history was validated.");
        assertOrThrow(!failFast || sawAnyHistoryRefillAfterArchive, "Final validation failed: new season history never refilled after archive reset.");

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