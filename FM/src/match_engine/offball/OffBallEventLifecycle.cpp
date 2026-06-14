#include"fm/match_engine/offball/OffBallEventLifecycle.h"

#include<algorithm>

namespace {
    const TeamSimState* teamFor(const MatchSimulationState& state, TeamId teamId) {
        return findTeamState(state, teamId);
    }

    bool shouldExpireForPhase(const TeamSimState* team, const OffBallSupportEvent& event) {
        if (team == nullptr || !event.expiresOnPhaseChange) {
            return false;
        }
        if (team->currentPhase == MatchTeamPhase::DefensiveTransition
            || team->currentPhase == MatchTeamPhase::SettledDefense) {
            return isAttackingSupportEvent(event.eventType);
        }
        return team->currentPhase != event.sourcePhase
            && event.sourcePhase != MatchTeamPhase::CounterAttack;
    }

    OffBallEventCompletionReason expirationReason(
        const MatchSimulationState& state,
        const OffBallSupportEvent& event) {
        const TeamSimState* team = teamFor(state, event.teamId);
        const PlayerSimState* player = findPlayerState(state, event.playerId);

        if (event.expiresOnBallOut && state.ball.controlState == BallControlState::OutOfPlay) {
            return OffBallEventCompletionReason::BallOut;
        }
        if (event.expiresOnOpponentControl
            && state.ball.controlState == BallControlState::Controlled
            && state.ball.carrierTeamId != 0
            && state.ball.carrierTeamId != event.teamId) {
            return OffBallEventCompletionReason::OpponentControl;
        }
        if (event.expiresOnPossessionLoss
            && state.possession.teamInPossession != 0
            && state.possession.teamInPossession != event.teamId) {
            return OffBallEventCompletionReason::PossessionLoss;
        }
        if (shouldExpireForPhase(team, event)) {
            return OffBallEventCompletionReason::PhaseChange;
        }
        if (player != nullptr && player->hasBall) {
            return OffBallEventCompletionReason::BecameBallCarrier;
        }
        if (state.currentSecond - event.startSecond >= event.maxDurationSeconds) {
            return OffBallEventCompletionReason::Timeout;
        }
        return OffBallEventCompletionReason::None;
    }

    void finalizeMovementQuality(
        OffBallSupportEvent& event,
        const PlayerSimState* player) {
        if (player == nullptr) {
            event.distanceToTargetAtCompletion = event.distanceToTargetAtCreation;
            event.distanceMovedDuringEvent = 0.0;
            return;
        }
        event.distanceToTargetAtCompletion =
            PitchGeometry::distance(player->position, event.resolvedTargetPoint);
        event.distanceMovedDuringEvent =
            PitchGeometry::distance(event.startPosition, player->position);
    }
}

const std::vector<OffBallSupportEvent>& OffBallEventLifecycle::activeEvents() const {
    return activeEvents_;
}

std::vector<OffBallSupportEvent> OffBallEventLifecycle::activeEventsForTeam(TeamId teamId) const {
    std::vector<OffBallSupportEvent> events;
    for (const OffBallSupportEvent& event : activeEvents_) {
        if (event.teamId == teamId) {
            events.push_back(event);
        }
    }
    return events;
}

const OffBallSupportEvent* OffBallEventLifecycle::activeEventForPlayer(PlayerId playerId) const {
    for (const OffBallSupportEvent& event : activeEvents_) {
        if (event.playerId == playerId) {
            return &event;
        }
    }
    return nullptr;
}

bool OffBallEventLifecycle::hasActiveEventForPlayer(PlayerId playerId) const {
    return activeEventForPlayer(playerId) != nullptr;
}

bool OffBallEventLifecycle::hadRecentSupportEvent(
    PlayerId playerId,
    int currentSecond,
    int windowSeconds) const {
    if (hasActiveEventForPlayer(playerId)) {
        return true;
    }
    for (const OffBallSupportEvent& event : recentEvents_) {
        if (event.playerId == playerId
            && currentSecond - event.completedSecond <= windowSeconds) {
            return true;
        }
    }
    return false;
}

bool OffBallEventLifecycle::hadCompletedSupportEventWithin(
    PlayerId playerId,
    int currentSecond,
    int windowSeconds) const {
    for (const OffBallSupportEvent& event : recentEvents_) {
        if (event.playerId == playerId
            && currentSecond - event.completedSecond <= windowSeconds) {
            return true;
        }
    }
    return false;
}

bool OffBallEventLifecycle::hadImmediateSupportCompletion(
    PlayerId playerId,
    int currentSecond,
    int windowSeconds) const {
    for (const OffBallSupportEvent& event : recentEvents_) {
        if (event.playerId == playerId
            && event.completionReason == OffBallEventCompletionReason::ReachedRegion
            && currentSecond - event.completedSecond <= windowSeconds) {
            return true;
        }
    }
    return false;
}

OffBallLifecycleResult OffBallEventLifecycle::addEvents(
    const std::vector<OffBallSupportEvent>& events,
    int currentSecond) {
    OffBallLifecycleResult result;
    for (OffBallSupportEvent event : events) {
        if (event.playerId == 0 || event.teamId == 0 || event.eventType == OffBallEventType::None) {
            continue;
        }
        auto existing = std::find_if(
            activeEvents_.begin(),
            activeEvents_.end(),
            [&event](const OffBallSupportEvent& active) {
                return active.playerId == event.playerId;
            });
        if (existing != activeEvents_.end()) {
            if (existing->priority > event.priority) {
                continue;
            }
            existing->completed = true;
            existing->completionReason = OffBallEventCompletionReason::Replaced;
            existing->completedSecond = currentSecond;
            result.expired.push_back(*existing);
            remember(*existing);
            activeEvents_.erase(existing);
        }
        activeEvents_.push_back(event);
    }
    return result;
}

OffBallLifecycleResult OffBallEventLifecycle::update(
    const MatchSimulationState& state,
    const RestDefenseModel&) {
    OffBallLifecycleResult result;
    std::vector<OffBallSupportEvent> retained;
    retained.reserve(activeEvents_.size());

    for (OffBallSupportEvent event : activeEvents_) {
        if (const PlayerSimState* player = findPlayerState(state, event.playerId)) {
            if (event.targetRegion.contains(player->position)
                || PitchGeometry::distance(player->position, event.resolvedTargetPoint) <= 1.5) {
                event.completed = true;
                event.completionReason = OffBallEventCompletionReason::ReachedRegion;
                event.completedSecond = state.currentSecond;
                finalizeMovementQuality(event, player);
                result.completed.push_back(event);
                remember(event);
                continue;
            }
        }

        const OffBallEventCompletionReason reason = expirationReason(state, event);
        if (reason != OffBallEventCompletionReason::None) {
            event.completed = false;
            event.completionReason = reason;
            event.completedSecond = state.currentSecond;
            finalizeMovementQuality(event, findPlayerState(state, event.playerId));
            result.expired.push_back(event);
            remember(event);
            continue;
        }

        retained.push_back(event);
    }

    activeEvents_ = retained;
    recentEvents_.erase(
        std::remove_if(
            recentEvents_.begin(),
            recentEvents_.end(),
            [&state](const OffBallSupportEvent& event) {
                return state.currentSecond - event.completedSecond > 20;
            }),
        recentEvents_.end());
    return result;
}

OffBallLifecycleResult OffBallEventLifecycle::expireForShot(
    TeamId shootingTeamId,
    int currentSecond) {
    OffBallLifecycleResult result;
    std::vector<OffBallSupportEvent> retained;
    retained.reserve(activeEvents_.size());
    for (OffBallSupportEvent event : activeEvents_) {
        if (event.teamId == shootingTeamId && event.expiresOnShot) {
            event.completed = false;
            event.completionReason = OffBallEventCompletionReason::Shot;
            event.completedSecond = currentSecond;
            result.expired.push_back(event);
            remember(event);
        } else {
            retained.push_back(event);
        }
    }
    activeEvents_ = retained;
    return result;
}

OffBallLifecycleResult OffBallEventLifecycle::expireForBallOut(int currentSecond) {
    OffBallLifecycleResult result;
    for (OffBallSupportEvent event : activeEvents_) {
        if (event.expiresOnBallOut) {
            event.completed = false;
            event.completionReason = OffBallEventCompletionReason::BallOut;
            event.completedSecond = currentSecond;
            result.expired.push_back(event);
            remember(event);
        }
    }
    activeEvents_.erase(
        std::remove_if(
            activeEvents_.begin(),
            activeEvents_.end(),
            [](const OffBallSupportEvent& event) {
                return event.expiresOnBallOut;
            }),
        activeEvents_.end());
    return result;
}

void OffBallEventLifecycle::remember(const OffBallSupportEvent& event) {
    recentEvents_.push_back(event);
    if (recentEvents_.size() > 64) {
        recentEvents_.erase(recentEvents_.begin());
    }
}
