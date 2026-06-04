#include"fm/match_engine/ball/LooseBallRecoveryModel.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>
#include<limits>

namespace {
    struct LooseBallRecoveryProfile {
        double immediateControlSeconds = 0.55;
        double maxRaceWindowSeconds = 3.25;
        double controlRangeMeters = 1.85;
        double minimumSpeedMetersPerSecond = 0.5;
    };

    double movementDurationSeconds(
        PitchPoint from,
        PitchPoint to,
        double speedMetersPerSecond,
        double minimum,
        double maximum) {
        const double distance = PitchGeometry::distance(from, to);
        const double safeSpeed =
            std::max(speedMetersPerSecond, LooseBallRecoveryProfile{}.minimumSpeedMetersPerSecond);
        return std::clamp(distance / safeSpeed, minimum, maximum);
    }

    double reactionBonusFor(const LooseBallRecoveryCandidate& candidate, TeamId lastPossessionTeamId) {
        double bonus = 0.0;
        if (candidate.currentIntent.type == PlayerIntentType::RecoverLooseBall) {
            bonus += 0.22;
        }
        if (candidate.currentIntent.type == PlayerIntentType::PressBallCarrier
            || candidate.currentIntent.type == PlayerIntentType::ContainBallCarrier
            || candidate.currentIntent.type == PlayerIntentType::BlockPassingLane
            || candidate.currentIntent.type == PlayerIntentType::InterceptBallPath) {
            bonus += 0.12;
        }
        if (lastPossessionTeamId != 0 && candidate.teamId != lastPossessionTeamId) {
            bonus += 0.08;
        }
        return bonus;
    }
}

LooseBallRecoveryResult LooseBallRecoveryModel::resolve(
    const LooseBallRecoveryContext& context) const {
    const LooseBallRecoveryProfile profile;
    const LooseBallRecoveryCandidate* best = nullptr;
    double bestAdjustedArrival = std::numeric_limits<double>::max();
    double bestDistance = std::numeric_limits<double>::max();

    for (const LooseBallRecoveryCandidate& candidate : context.candidates) {
        if (candidate.playerId == 0 || candidate.teamId == 0) {
            continue;
        }

        const double distance = PitchGeometry::distance(candidate.position, context.loosePoint);
        const double rawArrival = movementDurationSeconds(
            candidate.position,
            context.loosePoint,
            candidate.effectivePace,
            0.1,
            6.0);
        const double attributeReaction =
            (std::clamp(static_cast<double>(candidate.baseOverall), 0.0, 100.0) - 50.0) / 180.0
            + std::clamp(candidate.effectiveAcceleration, 0.0, 9.0) / 90.0;
        const double adjustedArrival = std::max(
            0.05,
            rawArrival - reactionBonusFor(candidate, context.lastPossessionTeamId) - attributeReaction);

        if (adjustedArrival < bestAdjustedArrival
            || (std::abs(adjustedArrival - bestAdjustedArrival) < 0.001
                && distance < bestDistance)) {
            best = &candidate;
            bestAdjustedArrival = adjustedArrival;
            bestDistance = distance;
        }
    }

    if (best == nullptr) {
        return LooseBallRecoveryResult{};
    }

    return LooseBallRecoveryResult{
        best->playerId,
        best->teamId,
        std::min(bestAdjustedArrival, profile.maxRaceWindowSeconds),
        bestDistance <= profile.controlRangeMeters
            || bestAdjustedArrival <= profile.immediateControlSeconds
    };
}
