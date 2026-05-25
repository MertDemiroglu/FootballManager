#pragma once

#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<vector>

struct ActionCandidateGenerationRequest {
    const MatchTeamSnapshot* teamSnapshot = nullptr;
    const MatchTeamSnapshot* opponentSnapshot = nullptr;
    PlayerSimState ballCarrier;
    BallState ballState;
    const MatchSimulationState& simulationState;
    TeamShapeContext teamShapeContext;
    std::vector<PlayerSimState> teammates;
    std::vector<PlayerSimState> opponents;
};

class ActionCandidateGenerator {
public:
    std::vector<ActionCandidate> generate(
        const ActionCandidateGenerationRequest& request) const;
};
