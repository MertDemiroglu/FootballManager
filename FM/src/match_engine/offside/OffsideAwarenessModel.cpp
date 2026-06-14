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

    double passerOffsideVision(const PlayerAttributes& attributes) {
        return std::clamp(
            attributes.mental.vision * 0.34
                + attributes.mental.decisions * 0.30
                + attributes.technical.passing * 0.20
                + attributes.mental.composure * 0.16,
            0.0,
            100.0);
    }

    bool isOpponentHalf(PitchPoint point, AttackingDirection direction) {
        return progress(point, direction) > PitchGeometry::LengthMeters / 2.0;
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

OffsideRiskResult OffsideAwarenessModel::evaluateRisk(
    const OffsideRiskRequest& request) const {
    OffsideRiskResult result;
    if (!request.line.valid || !isOpponentHalf(request.receiverPosition, request.attackingDirection)) {
        return result;
    }

    const double receiverProgress = progress(request.receiverPosition, request.attackingDirection);
    const double targetProgress = progress(request.targetPoint, request.attackingDirection);
    const double ballProgress = progress(request.ballPosition, request.attackingDirection);
    const double activeProgress = std::max(receiverProgress, targetProgress);
    result.receiverDistanceToLine = request.line.lineProgress - receiverProgress;
    if (activeProgress <= ballProgress + 0.25
        || activeProgress <= request.line.lineProgress + tuning_.lineBufferMeters) {
        return result;
    }

    result.risky = true;
    result.distanceBeyondLine = activeProgress - request.line.lineProgress;
    const double receiverAwareness = awarenessScore(request.receiverAttributes);
    const double passerVision = passerOffsideVision(request.passerAttributes);
    const double mitigation = std::clamp((receiverAwareness + passerVision) / 200.0, 0.0, 1.0);
    result.riskScore = std::clamp(
        28.0 + result.distanceBeyondLine * 18.0 - mitigation * 30.0,
        0.0,
        100.0);
    return result;
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
    result.checked = phase <= 0.65;
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
