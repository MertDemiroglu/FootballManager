#pragma once

#include"fm/common/Date.h"
#include"fm/common/Types.h"
#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchEngineTypes.h"

#include<cstdint>

struct MatchEngineOptions {
    MatchSimulationDetail detail = MatchSimulationDetail::BackgroundSummary;
    std::uint64_t deterministicSeed = 0;
    double watchedStepSeconds = 1.0;
    double backgroundStepSeconds = 3.0;
};

struct MatchEngineInput {
    MatchId matchId = 0;
    LeagueId leagueId = 0;
    int seasonYear = 0;
    int matchweek = 0;
    Date matchDate{ 1900, Month::January, 1 };

    MatchTeamSnapshot homeTeam;
    MatchTeamSnapshot awayTeam;

    MatchEngineOptions options;
};
