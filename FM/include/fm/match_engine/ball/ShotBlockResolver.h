#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/ball/BallTrajectoryTypes.h"
#include"fm/match_engine/ball/ShotTypes.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>
#include<optional>
#include<vector>

struct ShotBlocker {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PitchPoint position;
    PlayerAttributes attributes;
    int baseOverall = 50;
};

struct ShotBlockRequest {
    BallTrajectory trajectory;
    ShotContext context;
    ShotQualityResult quality;
    ShotExecutionResult execution;
    std::vector<ShotBlocker> blockers;
    std::uint64_t seed = 0;
};

struct ShotBlockResult {
    bool blocked = false;
    PlayerId blockerPlayerId = 0;
    TeamId blockerTeamId = 0;
    PitchPoint contactPoint;
    double deflectionStrength = 0.0;
};

class ShotBlockResolver {
public:
    ShotBlockResult resolve(const ShotBlockRequest& request) const;
};
