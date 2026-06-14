#pragma once

#include"fm/match_engine/phase/MatchPhaseTypes.h"
#include"fm/match_engine/phase/TeamGameContext.h"

#include<string>

enum class CounterRejectionReason {
    None,
    NoCleanRegain,
    NoForwardLane,
    NoRunner,
    OpponentRecovered,
    SettledPossession
};

struct PhaseTransitionConfig {
    double minimumPhaseDurationSeconds = 24.0;
    double minimumCounterDurationSeconds = 6.0;
    double minimumControlledPossessionSeconds = 15.0;
    int minimumControlledPossessionActions = 5;
    int minimumFinalizingPossessionActions = 3;
    double finalizingEnterProgress = 0.62;
    double finalizingExitProgress = 0.50;
    double counterForwardLaneThreshold = 88.0;
    double counterMinimumBallProgress = 0.42;
    int counterMinimumRunnersAhead = 2;
    double counterExpireForwardLaneThreshold = 34.0;
    double defenseRecoveredThreatThreshold = 52.0;
    double defensiveTransitionThreatThreshold = 88.0;
    int defensiveTransitionExposedPlayersAhead = 7;
    double settledDefenseBreakThreatThreshold = 84.0;
    int finalizingSupportCount = 2;
    int defenseRecoveredPlayersBehindBall = 7;
};

struct PhaseTransitionResult {
    MatchTeamPhase phase = MatchTeamPhase::BuildUp;
    bool changed = false;
    bool heldForLooseBall = false;
    CounterRejectionReason counterRejection = CounterRejectionReason::None;
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
