#include"fm/match_engine/offside/OffsideAwarenessModel.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<cstdint>

namespace {
    double progress(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }

    PitchPoint pointAtProgress(
        PitchPoint point,
        double targetProgress,
        AttackingDirection direction) {
        PitchPoint adjusted = point;
        adjusted.x = direction == AttackingDirection::HomeToAway
            ? targetProgress
            : PitchGeometry::LengthMeters - targetProgress;
        return PitchGeometry::clampToPitch(adjusted);
    }

    double awarenessScore(const PlayerAttributes& attributes) {
        return std::clamp(
            attributes.mental.decisions * 0.28
                + attributes.mental.vision * 0.20
                + attributes.mental.positioning * 0.20
                + attributes.mental.offTheBall * 0.22
                + attributes.mental.concentration * 0.10,
            0.0,
            100.0);
    }

    double deterministicUnit(PlayerId playerId, int second) {
        std::uint64_t x = static_cast<std::uint64_t>(playerId) * 0x9e3779b97f4a7c15ULL
            ^ static_cast<std::uint64_t>(second + 31) * 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return static_cast<double>(x & 0xffffULL) / 65535.0;
    }
}

OffsideAwarenessResult OffsideAwarenessModel::evaluate(
    const OffsideAwarenessRequest& request) const {
    OffsideAwarenessResult result;
    result.targetPoint = request.targetPoint;
    if (!request.line.valid) {
        return result;
    }

    const double awareness = awarenessScore(request.attributes) / 100.0;
    result.checkIntervalSeconds =
        tuning_.poorCheckIntervalSeconds
        - (tuning_.poorCheckIntervalSeconds - tuning_.eliteCheckIntervalSeconds) * awareness;

    const double targetProgress = progress(request.targetPoint, request.attackingDirection);
    result.distanceToOffsideLine = request.line.lineProgress - targetProgress;
    const bool riskyTarget = targetProgress > request.line.lineProgress + tuning_.lineBufferMeters;
    if (!riskyTarget) {
        return result;
    }

    const double phase =
        std::fmod(
            static_cast<double>(request.currentSecond)
                + static_cast<double>(request.playerId % 11) * 0.13,
            std::max(0.1, result.checkIntervalSeconds));
    result.checked = phase <= 0.55;
    if (!result.checked) {
        result.failedToAdjust = true;
        return result;
    }

    const double adjustmentChance =
        tuning_.minimumAdjustmentChance
        + (tuning_.maximumAdjustmentChance - tuning_.minimumAdjustmentChance) * awareness;
    if (deterministicUnit(request.playerId, request.currentSecond) > adjustmentChance) {
        result.failedToAdjust = true;
        return result;
    }

    result.targetPoint = pointAtProgress(
        request.targetPoint,
        std::max(0.0, request.line.lineProgress - tuning_.safeMarginMeters),
        request.attackingDirection);
    result.adjusted = true;
    result.distanceToOffsideLine =
        request.line.lineProgress - progress(result.targetPoint, request.attackingDirection);
    return result;
}

