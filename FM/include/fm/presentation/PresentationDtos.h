#pragma once

#include "fm/common/Types.h"

#include <optional>
#include <string>
#include <vector>

struct TeamVisualDto {
    TeamId teamId = 0;
    std::string name;
    std::string primaryColor;
    std::string secondaryColor;
    std::string textColor;
};

struct LineupPlayerViewDto {
    int slotIndex = 0;
    int slotRoleKey = 0;
    PlayerId playerId = 0;
    bool hasPlayer = false;

    std::string playerName;
    std::string surname;
    std::string positionText;
    std::string roleText;
    std::string slotRoleText;

    int overall = 0;
    double pitchX = 0.5;
    double pitchY = 0.5;
    int displayOrder = 0;

    std::optional<double> matchRating;
};

struct LineupSummaryDto {
    int assignedPlayers = 0;
    std::optional<int> averageOverall;
    std::string averageOverallText;
};

struct TeamSheetViewDto {
    TeamVisualDto team;
    std::string coachName;
    std::string formationText;
    LineupSummaryDto summary;
    std::vector<LineupPlayerViewDto> players;
};

struct PreMatchViewDto {
    bool hasData = false;
    MatchId matchId = 0;
    int matchweek = 0;
    std::string dateText;

    TeamId homeTeamId = 0;
    TeamId awayTeamId = 0;
    TeamSheetViewDto home;
    TeamSheetViewDto away;

    std::string homeRecentForm;
    std::string awayRecentForm;
};

struct PostMatchViewDto {
    bool hasData = false;
    MatchId matchId = 0;
    int matchweek = 0;
    std::string dateText;

    TeamId homeTeamId = 0;
    TeamId awayTeamId = 0;
    int homeGoals = 0;
    int awayGoals = 0;

    TeamSheetViewDto home;
    TeamSheetViewDto away;
};
