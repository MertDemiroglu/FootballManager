#pragma once

#include"fm/match_engine/phase/MatchPhaseTypes.h"
#include"fm/match_engine/phase/TeamGameContext.h"

#include<string>

struct PhaseTransitionConfig {
    double minimumPhaseDurationSeconds = 10.0;
    double minimumCounterDurationSeconds = 6.0;
    double finalizingEnterProgress = 0.62;
    double finalizingExitProgress = 0.50;
    double counterForwardLaneThreshold = 58.0;
    double counterExpireForwardLaneThreshold = 34.0;
    double defenseRecoveredThreatThreshold = 34.0;
    double settledDefenseBreakThreatThreshold = 70.0;
    int finalizingSupportCount = 2;
    int defenseRecoveredPlayersBehindBall = 7;
};

struct PhaseTransitionResult {
    MatchTeamPhase phase = MatchTeamPhase::BuildUp;
    bool changed = false;
    std::string entryReason;
    std::string exitReason;
};

class PhaseTransitionModel {
public:
    explicit PhaseTransitionModel(PhaseTransitionConfig config = PhaseTransitionConfig{});

    PhaseTransitionResult evaluate(const TeamGameContext& context) const;

private:
    PhaseTransitionConfig config_;
};
