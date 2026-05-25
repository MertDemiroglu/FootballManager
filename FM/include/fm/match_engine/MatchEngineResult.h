#pragma once

#include"fm/common/Types.h"
#include"fm/match/MatchReport.h"
#include"fm/match_engine/MatchEngineTypes.h"

#include<optional>
#include<vector>

struct MatchTeamSimulationStats {
    TeamId teamId = 0;
    int goals = 0;
    int shots = 0;
    int shotsOnTarget = 0;
    int passesAttempted = 0;
    int passesCompleted = 0;
    int tacklesAttempted = 0;
    int tacklesWon = 0;
    int interceptions = 0;
    int fouls = 0;
    int corners = 0;
    int yellowCards = 0;
    int redCards = 0;
    double possessionShare = 0.0;
    double expectedGoals = 0.0;
};

struct MatchPlayerSimulationStats {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    int minutesPlayed = 0;
    int goals = 0;
    int assists = 0;
    int shots = 0;
    int passesAttempted = 0;
    int passesCompleted = 0;
    int tackles = 0;
    int interceptions = 0;
    int fouls = 0;
    int yellowCards = 0;
    int redCards = 0;
    double rating = 6.0;
};

struct MatchEngineResult {
    std::optional<MatchReport> report;
    MatchTeamSimulationStats homeStats;
    MatchTeamSimulationStats awayStats;
    std::vector<MatchPlayerSimulationStats> playerStats;
    std::vector<MatchEventRecord> events;
    std::vector<MatchTraceFrame> traceFrames;
    int simulatedSeconds = 0;
};
