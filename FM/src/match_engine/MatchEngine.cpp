#include"fm/match_engine/MatchEngine.h"

#include"fm/match_engine/simulation/CoordinateMatchSimulator.h"
#include"fm/match_engine/simulation/FastMatchSummarySimulator.h"

bool hasValidTeams(const MatchEngineInput& input) {
    return input.matchId != 0
        && input.homeTeam.teamId != 0
        && input.awayTeam.teamId != 0
        && input.homeTeam.teamId != input.awayTeam.teamId;
}

MatchEngineResult MatchEngine::simulate(const MatchEngineInput& input) const {
    if (!hasValidTeams(input)) {
        return {};
    }

    if (input.options.detail == MatchSimulationDetail::BackgroundSummary) {
        return FastMatchSummarySimulator{}.run(input);
    }

    return CoordinateMatchSimulator{}.run(input);
}
