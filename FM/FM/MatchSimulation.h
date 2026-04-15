#pragma once

#include"Date.h"
#include"MatchReport.h"
#include "TeamSheet.h"

class Team;

namespace MatchSimulation {
    MatchReport buildStrengthBasedReport(
        MatchId matchId,
        LeagueId leagueId,
        int seasonYear,
        int matchweek,
        const Team& homeTeam,
        const Team& awayTeam,
        const TeamSheet& homeSheet,
        const TeamSheet& awaySheet,
        const Date& date);
}
