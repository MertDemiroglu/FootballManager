#include"fm/match_engine/ball/PassResolutionFlow.h"

PassResolutionFlowResult PassResolutionFlow::resolve(
    const PassResolutionFlowRequest& request) const {
    return PassResolutionFlowResult{
        PassResolutionModel{}.resolve(request.context),
        request.receiverPlayerId,
        request.receiverTeamId,
        request.defenderPlayerId,
        request.defenderTeamId
    };
}
