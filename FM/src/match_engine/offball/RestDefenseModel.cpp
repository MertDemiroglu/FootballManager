#include"fm/match_engine/offball/RestDefenseModel.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>

namespace {
    double progress(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }

    bool isCenterBack(FormationSlotRole role) {
        return role == FormationSlotRole::CenterBack;
    }

    bool isFullback(FormationSlotRole role) {
        return role == FormationSlotRole::LeftBack
            || role == FormationSlotRole::RightBack
            || role == FormationSlotRole::LeftWingBack
            || role == FormationSlotRole::RightWingBack;
    }

    bool isDefensiveMidfielder(FormationSlotRole role) {
        return role == FormationSlotRole::DefensiveMidfielder;
    }

    bool isAdvancedFullbackEvent(OffBallEventType type) {
        return type == OffBallEventType::OverlapRun
            || type == OffBallEventType::UnderlapRun;
    }

    const PlayerGameContext* contextFor(
        const std::vector<PlayerGameContext>& contexts,
        PlayerId playerId) {
        for (const PlayerGameContext& context : contexts) {
            if (context.playerId == playerId) {
                return &context;
            }
        }
        return nullptr;
    }
}

RestDefenseSnapshot RestDefenseModel::evaluate(
    const RestDefenseEvaluationRequest& request) const {
    RestDefenseSnapshot snapshot;
    if (request.team == nullptr || request.playerContexts == nullptr) {
        return snapshot;
    }

    const double ballProgress = progress(request.ballPosition, request.attackingDirection);
    for (const PlayerSimState& player : request.team->players) {
        const PlayerGameContext* context = contextFor(*request.playerContexts, player.playerId);
        if (context == nullptr) {
            continue;
        }

        const double playerProgress = progress(player.position, request.attackingDirection);
        const bool behindBall = playerProgress <= ballProgress - 1.0;
        if (behindBall) {
            ++snapshot.playersBehindBall;
            if (isCenterBack(context->role)) {
                ++snapshot.centerBacksBehindBall;
            }
        }

        if (isFullback(context->role) && playerProgress >= ballProgress + 3.0) {
            ++snapshot.advancedFullbacks;
        }
    }

    for (const OffBallSupportEvent& event : request.activeEvents) {
        if (event.teamId != request.team->teamId) {
            continue;
        }
        const PlayerGameContext* context = contextFor(*request.playerContexts, event.playerId);
        if (context == nullptr) {
            continue;
        }
        if (isFullback(context->role) && isAdvancedFullbackEvent(event.eventType)) {
            ++snapshot.advancedFullbacks;
            if (context->isBallSideFullback) {
                snapshot.ballSideFullbackAdvanced = true;
            }
        }
    }

    bool hasFbOrDmBehind = false;
    for (const PlayerSimState& player : request.team->players) {
        const PlayerGameContext* context = contextFor(*request.playerContexts, player.playerId);
        if (context == nullptr) {
            continue;
        }
        const double playerProgress = progress(player.position, request.attackingDirection);
        if (playerProgress <= ballProgress - 1.0
            && (isFullback(context->role) || isDefensiveMidfielder(context->role))) {
            hasFbOrDmBehind = true;
            break;
        }
    }

    snapshot.farSideFullbackShouldHold =
        snapshot.ballSideFullbackAdvanced || snapshot.advancedFullbacks > 0;
    snapshot.stable =
        snapshot.centerBacksBehindBall >= 2
        && hasFbOrDmBehind
        && snapshot.advancedFullbacks <= 1;
    return snapshot;
}

bool RestDefenseModel::canApproveAdvancedRun(
    const RestDefenseSnapshot& snapshot,
    const PlayerGameContext& player,
    OffBallEventType eventType) const {
    if (!isAdvancedFullbackEvent(eventType)) {
        return true;
    }
    if (!player.isBallSideFullback) {
        return false;
    }
    if (snapshot.advancedFullbacks >= 1 || snapshot.ballSideFullbackAdvanced) {
        return false;
    }
    return snapshot.centerBacksBehindBall >= 2
        && snapshot.playersBehindBall >= 3;
}
