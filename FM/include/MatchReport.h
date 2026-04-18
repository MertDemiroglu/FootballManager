#pragma once

#include<vector>

#include"fm/common/Date.h"
#include"fm/roster/Formation.h"
#include"fm/common/Types.h"

enum class MatchEventKind {
    Goal,
    YellowCard,
    RedCard
};

struct MatchPlayerReport {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    bool started = false;
    int minutesPlayed = 0;
    int goals = 0;
    int assists = 0;
    int yellowCards = 0;
    int redCards = 0;
};

struct MatchEventRecord {
    int minute = 0;
    MatchEventKind kind = MatchEventKind::Goal;
    TeamId teamId = 0;
    PlayerId primaryPlayerId = 0;
    PlayerId secondaryPlayerId = 0;
};

struct MatchLineupSnapshot {
    TeamId teamId = 0;
    CoachId coachId = 0;
    FormationId formation = FormationId::FourFourTwo;
    std::vector<PlayerId> startingPlayerIds;
};

struct MatchReport {
    MatchId matchId = 0;
    LeagueId leagueId = 0;
    int seasonYear = 0;
    Date date{ 1900, Month::January, 1 };
    TeamId homeId = 0;
    TeamId awayId = 0;
    int matchweek = 0;
    int homeGoals = 0;
    int awayGoals = 0;
    MatchLineupSnapshot homeLineup;
    MatchLineupSnapshot awayLineup;
    std::vector<MatchPlayerReport> playerReports;
    std::vector<MatchEventRecord> events;
};
