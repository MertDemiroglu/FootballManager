#pragma once

#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/offball/OffBallSupportEvent.h"
#include"fm/match_engine/offball/RestDefenseModel.h"

#include<vector>

struct OffBallLifecycleResult {
    std::vector<OffBallSupportEvent> completed;
    std::vector<OffBallSupportEvent> expired;
};

class OffBallEventLifecycle {
public:
    const std::vector<OffBallSupportEvent>& activeEvents() const;
    std::vector<OffBallSupportEvent> activeEventsForTeam(TeamId teamId) const;
    const OffBallSupportEvent* activeEventForPlayer(PlayerId playerId) const;
    bool hasActiveEventForPlayer(PlayerId playerId) const;
    bool hadRecentSupportEvent(PlayerId playerId, int currentSecond, int windowSeconds = 12) const;
    bool hadCompletedSupportEventWithin(PlayerId playerId, int currentSecond, int windowSeconds) const;
    bool hadImmediateSupportCompletion(PlayerId playerId, int currentSecond, int windowSeconds = 2) const;

    OffBallLifecycleResult addEvents(
        const std::vector<OffBallSupportEvent>& events,
        int currentSecond);

    OffBallLifecycleResult update(
        const MatchSimulationState& state,
        const RestDefenseModel& restDefenseModel);

    OffBallLifecycleResult expireForShot(TeamId shootingTeamId, int currentSecond);
    OffBallLifecycleResult expireForBallOut(int currentSecond);

private:
    std::vector<OffBallSupportEvent> activeEvents_;
    std::vector<OffBallSupportEvent> recentEvents_;

    void remember(const OffBallSupportEvent& event);
};
