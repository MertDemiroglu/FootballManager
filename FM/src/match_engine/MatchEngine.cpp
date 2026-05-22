#include"fm/match_engine/MatchEngine.h"

bool hasValidTeams(const MatchEngineInput& input) {
    return input.matchId != 0
        && input.homeTeam.teamId != 0
        && input.awayTeam.teamId != 0
        && input.homeTeam.teamId != input.awayTeam.teamId;
}

MatchEngineResult MatchEngine::simulate(const MatchEngineInput& input) const {
    MatchEngineResult result;

    // Skeleton only. Later phases will build a MatchReport and optional trace data
    // from snapshot input, without mutating domain objects or applying results.
    if (!hasValidTeams(input)) {
        return result;
    }

    result.homeStats.teamId = input.homeTeam.teamId;
    result.awayStats.teamId = input.awayTeam.teamId;
    return result;
}
