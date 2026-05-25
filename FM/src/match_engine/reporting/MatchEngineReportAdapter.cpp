#include"fm/match_engine/reporting/MatchEngineReportAdapter.h"

#include<algorithm>
#include<vector>

namespace {
    std::vector<PlayerId> startingPlayerIdsFor(const TeamSheet& teamSheet) {
        std::vector<PlayerId> starters;

        if (!teamSheet.startingAssignments.empty()) {
            starters.reserve(teamSheet.startingAssignments.size());
            for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
                if (assignment.playerId != 0
                    && std::find(starters.begin(), starters.end(), assignment.playerId)
                        == starters.end()) {
                    starters.push_back(assignment.playerId);
                }
            }
            return starters;
        }

        starters.reserve(teamSheet.startingPlayerIds.size());
        for (PlayerId playerId : teamSheet.startingPlayerIds) {
            if (playerId != 0
                && std::find(starters.begin(), starters.end(), playerId) == starters.end()) {
                starters.push_back(playerId);
            }
        }
        return starters;
    }

    MatchLineupSnapshot buildLineupSnapshot(const MatchTeamSnapshot& team) {
        MatchLineupSnapshot lineup;
        lineup.teamId = team.teamId;
        lineup.coachId = team.teamSheet.coachId;
        lineup.formation = team.teamSheet.formation;
        lineup.startingPlayerIds = startingPlayerIdsFor(team.teamSheet);
        return lineup;
    }

    bool containsPlayerId(const std::vector<PlayerId>& playerIds, PlayerId playerId) {
        return std::find(playerIds.begin(), playerIds.end(), playerId) != playerIds.end();
    }

    bool startedFor(const MatchReport& report, TeamId teamId, PlayerId playerId) {
        if (teamId == report.homeLineup.teamId) {
            return containsPlayerId(report.homeLineup.startingPlayerIds, playerId);
        }
        if (teamId == report.awayLineup.teamId) {
            return containsPlayerId(report.awayLineup.startingPlayerIds, playerId);
        }
        return false;
    }

    MatchPlayerReport* findPlayerReport(
        std::vector<MatchPlayerReport>& reports,
        TeamId teamId,
        PlayerId playerId) {
        for (MatchPlayerReport& report : reports) {
            if (report.teamId == teamId && report.playerId == playerId) {
                return &report;
            }
        }
        return nullptr;
    }

    MatchPlayerReport& ensurePlayerReport(
        std::vector<MatchPlayerReport>& reports,
        TeamId teamId,
        PlayerId playerId) {
        if (MatchPlayerReport* existing = findPlayerReport(reports, teamId, playerId)) {
            return *existing;
        }

        MatchPlayerReport report;
        report.playerId = playerId;
        report.teamId = teamId;
        reports.push_back(report);
        return reports.back();
    }

    void appendStarterReports(
        MatchReport& report,
        const MatchLineupSnapshot& lineup) {
        for (PlayerId playerId : lineup.startingPlayerIds) {
            MatchPlayerReport& playerReport =
                ensurePlayerReport(report.playerReports, lineup.teamId, playerId);
            playerReport.started = true;
        }
    }

    MatchTeamReportStats toReportStats(const MatchTeamSimulationStats& stats) {
        MatchTeamReportStats reportStats;
        reportStats.goals = stats.goals;
        reportStats.shots = stats.shots;
        reportStats.shotsOnTarget = stats.shotsOnTarget;
        reportStats.passesAttempted = stats.passesAttempted;
        reportStats.passesCompleted = stats.passesCompleted;
        reportStats.tacklesAttempted = stats.tacklesAttempted;
        reportStats.tacklesWon = stats.tacklesWon;
        reportStats.interceptions = stats.interceptions;
        reportStats.fouls = stats.fouls;
        reportStats.corners = stats.corners;
        reportStats.yellowCards = stats.yellowCards;
        reportStats.redCards = stats.redCards;
        reportStats.possessionShare = stats.possessionShare;
        reportStats.expectedGoals = stats.expectedGoals;
        return reportStats;
    }
}

MatchReport MatchEngineReportAdapter::buildReport(
    const MatchEngineInput& input,
    const MatchEngineResult& result) const {
    MatchReport report;
    report.matchId = input.matchId;
    report.leagueId = input.leagueId;
    report.seasonYear = input.seasonYear;
    report.date = input.matchDate;
    report.homeId = input.homeTeam.teamId;
    report.awayId = input.awayTeam.teamId;
    report.matchweek = input.matchweek;
    report.homeGoals = result.homeStats.goals;
    report.awayGoals = result.awayStats.goals;
    report.homeStats = toReportStats(result.homeStats);
    report.awayStats = toReportStats(result.awayStats);
    report.homeLineup = buildLineupSnapshot(input.homeTeam);
    report.awayLineup = buildLineupSnapshot(input.awayTeam);

    report.playerReports.reserve(
        result.playerStats.size()
        + report.homeLineup.startingPlayerIds.size()
        + report.awayLineup.startingPlayerIds.size());

    for (const MatchPlayerSimulationStats& stats : result.playerStats) {
        if (stats.playerId == 0) {
            continue;
        }

        MatchPlayerReport& playerReport =
            ensurePlayerReport(report.playerReports, stats.teamId, stats.playerId);
        playerReport.started = startedFor(report, stats.teamId, stats.playerId);
        playerReport.minutesPlayed = stats.minutesPlayed;
        playerReport.goals = stats.goals;
        playerReport.assists = stats.assists;
        playerReport.yellowCards = stats.yellowCards;
        playerReport.redCards = stats.redCards;
        playerReport.rating = stats.rating;
    }

    appendStarterReports(report, report.homeLineup);
    appendStarterReports(report, report.awayLineup);

    report.events = result.events;

    return report;
}
