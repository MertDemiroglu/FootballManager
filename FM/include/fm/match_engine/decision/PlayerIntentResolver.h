#pragma once

#include"fm/match/TeamSheet.h"
#include"fm/match/TacticalSetup.h"
#include"fm/match_engine/contest/ContestResolver.h"
#include"fm/match_engine/MatchSimulationState.h"
#include"fm/match_engine/decision/DecisionContext.h"
#include"fm/match_engine/geometry/TacticalZones.h"
#include"fm/match_engine/movement/TeamShapeModel.h"

#include<cstdint>
#include<optional>
#include<vector>

enum class IntentTeamMode {
    Attacking,
    Defending,
    NeutralBall
};

struct PlayerIntentResolutionContext {
    TeamId teamId = 0;
    IntentTeamMode teamMode = IntentTeamMode::Defending;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;
    TacticalSetup tacticalSetup;

    BallState ballState;
    std::optional<ContestResolverResult> lastContestResult;

    std::vector<PlayerShapeTarget> shapeTargets;

    std::vector<PlayerSimState> teammates;
    std::vector<PlayerSimState> opponents;

    std::vector<TeamSheetSlotAssignment> teamAssignments;
    std::vector<TeamSheetSlotAssignment> opponentAssignments;
    DefensiveContext defensiveContext;

    std::uint64_t seed = 0;
};

struct ResolvedPlayerIntent {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PlayerIntent intent;
    PitchPoint finalTarget;
    TacticalZone currentZone = TacticalZone::Unknown;
    TacticalZone targetZone = TacticalZone::Unknown;
    double score = 0.0;
};

class PlayerIntentResolver {
public:
    std::vector<ResolvedPlayerIntent> resolveTeamIntents(
        const PlayerIntentResolutionContext& context) const;
};
