#pragma once

#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/offball/OffBallSupportEvent.h"
#include"fm/match_engine/phase/PlayerGameContext.h"

#include<vector>

struct RestDefenseSnapshot {
    bool stable = false;
    int playersBehindBall = 0;
    int centerBacksBehindBall = 0;
    bool ballSideFullbackAdvanced = false;
    bool farSideFullbackShouldHold = false;
    int advancedFullbacks = 0;
};

struct RestDefenseEvaluationRequest {
    const TeamSimState* team = nullptr;
    const std::vector<PlayerGameContext>* playerContexts = nullptr;
    std::vector<OffBallSupportEvent> activeEvents;
    PitchPoint ballPosition;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
};

class RestDefenseModel {
public:
    RestDefenseSnapshot evaluate(const RestDefenseEvaluationRequest& request) const;

    bool canApproveAdvancedRun(
        const RestDefenseSnapshot& snapshot,
        const PlayerGameContext& player,
        OffBallEventType eventType) const;
};
