#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/ball/PassResolutionModel.h"

struct PassResolutionFlowRequest {
    PassResolutionContext context;
    PlayerId receiverPlayerId = 0;
    TeamId receiverTeamId = 0;
    PlayerId defenderPlayerId = 0;
    TeamId defenderTeamId = 0;
};

struct PassResolutionFlowResult {
    PassResolutionResult resolution;
    PlayerId receiverPlayerId = 0;
    TeamId receiverTeamId = 0;
    PlayerId defenderPlayerId = 0;
    TeamId defenderTeamId = 0;
};

class PassResolutionFlow {
public:
    PassResolutionFlowResult resolve(const PassResolutionFlowRequest& request) const;
};
