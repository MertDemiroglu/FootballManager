#pragma once

#include"fm/match/TeamSheet.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/phase/PhaseTransitionModel.h"
#include"fm/match_engine/phase/PlayerGameContext.h"
#include"fm/match_engine/phase/TeamGameContext.h"

#include<vector>

class MatchPhaseModel {
public:
    explicit MatchPhaseModel(PhaseTransitionConfig config = PhaseTransitionConfig{});

    PhaseTransitionResult evaluateTeamPhase(const TeamGameContext& context) const;

    TeamGameContext buildTeamContext(
        const TeamSimState& team,
        const TeamSimState& opponent,
        const MatchSimulationState& state,
        const std::vector<PlayerShapeTarget>& teamTargets,
        const std::vector<PlayerShapeTarget>& opponentTargets) const;

    std::vector<PlayerGameContext> buildPlayerContexts(
        const TeamSimState& team,
        const TeamSimState& opponent,
        const TeamSheet& teamSheet,
        const std::vector<PlayerShapeTarget>& teamTargets,
        PitchPoint ballPosition) const;

private:
    PhaseTransitionConfig config_;
    PhaseTransitionModel transitionModel_;
};
