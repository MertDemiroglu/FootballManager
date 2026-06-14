#pragma once

#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/match_engine/MatchEngineSnapshots.h"
#include"fm/match_engine/decision/PhaseDecisionTuning.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/phase/PlayerGameContext.h"
#include"fm/match_engine/phase/TeamGameContext.h"
#include"fm/roster/FormationSlotRole.h"

#include<vector>

struct PhaseAwareDecisionContext {
    MatchTeamPhase phase = MatchTeamPhase::BuildUp;
    const MatchTeamSnapshot* teamSnapshot = nullptr;
    const TeamGameContext* teamGameContext = nullptr;
    const PlayerGameContext* carrierGameContext = nullptr;
    FormationSlotRole carrierRole = FormationSlotRole::Unknown;
    PitchPoint ballPosition;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
};

class PhaseAwareDecisionModel {
public:
    void adjustCandidates(
        const PhaseAwareDecisionContext& context,
        std::vector<ActionCandidate>& candidates) const;

private:
    PhaseDecisionTuning tuning_ = defaultPhaseDecisionTuning();
};
