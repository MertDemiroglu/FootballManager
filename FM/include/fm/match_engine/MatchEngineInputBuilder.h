#pragma once

#include"fm/match_engine/MatchEngineInput.h"

#include<cstdint>

class Team;

class MatchEngineInputBuilder {
public:
    MatchEngineInput build(
        MatchId matchId,
        LeagueId leagueId,
        int seasonYear,
        int matchweek,
        const Date& matchDate,
        const Team& homeTeam,
        const Team& awayTeam,
        const TeamSheet& homeSheet,
        const TeamSheet& awaySheet,
        MatchEngineOptions options = {}) const;

    MatchEngineInput build(
        MatchId matchId,
        LeagueId leagueId,
        const Date& matchDate,
        const Team& homeTeam,
        const Team& awayTeam,
        const TeamSheet& homeSheet,
        const TeamSheet& awaySheet,
        MatchEngineOptions options = {}) const;
};

std::uint64_t buildDeterministicMatchSeed(
    MatchId matchId,
    LeagueId leagueId,
    const Date& matchDate,
    TeamId homeTeamId,
    TeamId awayTeamId);
