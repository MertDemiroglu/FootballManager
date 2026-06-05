#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/MatchEngineTypes.h"

#include<vector>

struct LooseBallRecoveryCandidate {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PitchPoint position;
    PlayerIntent currentIntent;
    double effectivePace = 0.0;
    double effectiveAcceleration = 0.0;
    int baseOverall = 0;
};

struct LooseBallRecoveryContext {
    PitchPoint loosePoint;
    TeamId lastPossessionTeamId = 0;
    std::vector<LooseBallRecoveryCandidate> candidates;
};

struct LooseBallRecoveryResult {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    double elapsedSeconds = 1.0;
    bool controlled = false;
};

class LooseBallRecoveryModel {
public:
    LooseBallRecoveryResult resolve(const LooseBallRecoveryContext& context) const;
};
