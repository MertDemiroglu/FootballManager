#include "fm/presentation/MatchPresentationBuilder.h"

#include "fm/common/Date.h"
#include "fm/competition/League.h"
#include "fm/interaction/PostMatchInteraction.h"
#include "fm/interaction/PreMatchInteraction.h"
#include "fm/match/MatchReport.h"

#include <array>
#include <string>

namespace {
    std::string monthName(Month month) {
        static const std::array<const char*, 12> names{
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };
        const int index = static_cast<int>(month) - 1;
        if (index < 0 || index >= static_cast<int>(names.size())) {
            return "Unknown";
        }
        return names[static_cast<std::size_t>(index)];
    }

    std::string formatDate(const Date& date) {
        return monthName(date.getMonth()) + " "
            + std::to_string(date.getDay()) + ", "
            + std::to_string(date.getYear());
    }

    void applyMatchRatings(TeamSheetViewDto& sheet, const MatchReport& report) {
        for (LineupPlayerViewDto& player : sheet.players) {
            for (const MatchPlayerReport& playerReport : report.playerReports) {
                if (playerReport.playerId == player.playerId) {
                    player.matchRating = playerReport.rating;
                    break;
                }
            }
        }
    }
}

MatchPresentationBuilder::MatchPresentationBuilder(const TeamSheetPresentationBuilder& sheetBuilder)
    : sheetBuilder(sheetBuilder) {
}

// This builder is intentionally scoped to Pre/Post match presentation for now.
// DashboardPresentationBuilder and TransferPresentationBuilder can be added when those screens accumulate similar preparation logic.
PreMatchViewDto MatchPresentationBuilder::buildPreMatch(
    const PreMatchInteraction& interaction,
    const League* league) const {
    PreMatchViewDto dto;
    dto.hasData = league != nullptr;
    dto.matchId = interaction.getMatchId();
    dto.matchweek = interaction.getMatchweek();
    dto.dateText = formatDate(interaction.getDate());
    dto.homeTeamId = interaction.getHomeId();
    dto.awayTeamId = interaction.getAwayId();

    const Team* homeTeam = league ? league->findTeamById(interaction.getHomeId()) : nullptr;
    const Team* awayTeam = league ? league->findTeamById(interaction.getAwayId()) : nullptr;
    dto.home = sheetBuilder.buildFromTeamSheet(interaction.getHomeSheet(), homeTeam);
    dto.away = sheetBuilder.buildFromTeamSheet(interaction.getAwaySheet(), awayTeam);
    dto.homeRecentForm = league ? league->getRecentFormString(interaction.getHomeId(), 5) : std::string();
    dto.awayRecentForm = league ? league->getRecentFormString(interaction.getAwayId(), 5) : std::string();
    return dto;
}

PostMatchViewDto MatchPresentationBuilder::buildPostMatch(
    const PostMatchInteraction& interaction,
    const League* league) const {
    PostMatchViewDto dto;
    dto.hasData = league != nullptr;
    dto.matchId = interaction.getMatchId();
    dto.matchweek = interaction.getMatchweek();
    dto.dateText = formatDate(interaction.getDate());
    dto.homeTeamId = interaction.getHomeId();
    dto.awayTeamId = interaction.getAwayId();
    dto.homeGoals = interaction.getHomeGoals();
    dto.awayGoals = interaction.getAwayGoals();

    if (!league) {
        return dto;
    }

    if (const MatchReport* report = league->findCurrentSeasonMatchReportById(interaction.getMatchId())) {
        const Team* homeTeam = league->findTeamById(report->homeId);
        const Team* awayTeam = league->findTeamById(report->awayId);
        dto.homeTeamId = report->homeId;
        dto.awayTeamId = report->awayId;
        dto.home = sheetBuilder.buildFromMatchLineupSnapshot(report->homeLineup, homeTeam);
        dto.away = sheetBuilder.buildFromMatchLineupSnapshot(report->awayLineup, awayTeam);
        applyMatchRatings(dto.home, *report);
        applyMatchRatings(dto.away, *report);
        return dto;
    }

    dto.home = sheetBuilder.buildFromMatchLineupSnapshot({}, league->findTeamById(interaction.getHomeId()));
    dto.away = sheetBuilder.buildFromMatchLineupSnapshot({}, league->findTeamById(interaction.getAwayId()));
    return dto;
}
