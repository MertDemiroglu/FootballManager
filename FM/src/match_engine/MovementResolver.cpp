#include"fm/match_engine/MovementResolver.h"

#include"fm/match_engine/PitchGeometry.h"

#include<algorithm>
#include<cmath>

namespace {
    constexpr double DefaultSpeedMetersPerSecond = 6.0;
    constexpr double Epsilon = 0.001;
}

MovementResolutionResult MovementResolver::resolve(
    const MovementResolutionRequest& request) const {
    MovementResolutionResult result;
    result.playerId = request.player.playerId;
    result.teamId = request.player.teamId;
    result.previousPosition = PitchGeometry::clampToPitch(request.player.position);
    result.targetPosition = PitchGeometry::clampToPitch(request.target);
    result.newPosition = result.previousPosition;
    result.distanceToTargetBefore =
        PitchGeometry::distance(result.previousPosition, result.targetPosition);

    if (request.deltaSeconds <= 0.0 || result.distanceToTargetBefore <= Epsilon) {
        result.reachedTarget = result.distanceToTargetBefore <= Epsilon;
        return result;
    }

    const double baseSpeed = request.player.effectivePace > 0.0
        ? request.player.effectivePace
        : DefaultSpeedMetersPerSecond;
    const double fatigueFactor =
        1.0 - (std::clamp(request.player.fatigue, 0.0, 1.0) * 0.35);
    const double urgencyFactor =
        1.0 + (std::clamp(request.intent.urgency, 0.0, 1.0) * 0.15);
    const double maxDistance =
        std::max(0.0, baseSpeed) * fatigueFactor * urgencyFactor * request.deltaSeconds;
    result.distanceMoved = std::min(maxDistance, result.distanceToTargetBefore);

    if (result.distanceMoved <= Epsilon) {
        return result;
    }

    const double progress = result.distanceMoved / result.distanceToTargetBefore;
    result.newPosition = PitchGeometry::clampToPitch(PitchPoint{
        result.previousPosition.x
            + ((result.targetPosition.x - result.previousPosition.x) * progress),
        result.previousPosition.y
            + ((result.targetPosition.y - result.previousPosition.y) * progress)
    });
    result.reachedTarget = result.distanceMoved >= result.distanceToTargetBefore - Epsilon;
    return result;
}
