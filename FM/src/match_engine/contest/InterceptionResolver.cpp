#include"fm/match_engine/contest/InterceptionResolver.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>

namespace {
    constexpr double DefaultPlayerSpeedMetersPerSecond = 6.5;
    constexpr double MinimumPlayerSpeedMetersPerSecond = 0.5;

    double speedFor(const PlayerSimState& defender) {
        if (defender.effectivePace > 0.0) {
            return std::max(defender.effectivePace, MinimumPlayerSpeedMetersPerSecond);
        }

        return DefaultPlayerSpeedMetersPerSecond;
    }

    double qualityScoreFor(
        const PlayerSimState& defender,
        double arrivalMarginSeconds,
        double distanceMeters) {
        const double timingScore = std::max(0.0, 10.0 - (arrivalMarginSeconds * 10.0));
        const double distanceScore = std::max(0.0, 6.0 - (distanceMeters * 0.15));
        const double overallScore = std::clamp(static_cast<double>(defender.baseOverall), 0.0, 100.0) / 25.0;
        return timingScore + distanceScore + overallScore;
    }
}

InterceptionResolverResult InterceptionResolver::resolve(
    const InterceptionResolverRequest& request) const {
    InterceptionResolverResult result;
    const std::vector<BallTrajectorySample> samples =
        sampleTrajectory(request.trajectory, request.trajectorySampleCount);
    const double reactionWindowSeconds = std::max(0.0, request.reactionWindowSeconds);

    for (const PlayerSimState& defender : request.defenders) {
        if (defender.playerId == 0 || defender.teamId == 0) {
            continue;
        }

        const double playerSpeed = speedFor(defender);
        for (const BallTrajectorySample& sample : samples) {
            const double distanceMeters =
                PitchGeometry::distance(defender.position, sample.point);
            const double playerArrivalSecond =
                request.trajectory.startSecond + (distanceMeters / playerSpeed);
            const double arrivalMarginSeconds = playerArrivalSecond - sample.second;

            if (arrivalMarginSeconds > reactionWindowSeconds) {
                continue;
            }

            InterceptionCandidate candidate;
            candidate.playerId = defender.playerId;
            candidate.teamId = defender.teamId;
            candidate.interceptionPoint = sample.point;
            candidate.ballArrivalSecond = sample.second;
            candidate.playerArrivalSecond = playerArrivalSecond;
            candidate.arrivalMarginSeconds = arrivalMarginSeconds;
            candidate.qualityScore = qualityScoreFor(
                defender,
                arrivalMarginSeconds,
                distanceMeters);

            if (!result.bestCandidate
                || candidate.qualityScore > result.bestCandidate->qualityScore) {
                result.bestCandidate = candidate;
            }

            result.candidates.push_back(candidate);
        }
    }

    return result;
}
