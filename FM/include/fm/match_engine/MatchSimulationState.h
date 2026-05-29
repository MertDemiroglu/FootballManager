#pragma once

#include"fm/common/Types.h"
#include"fm/match/TacticalSetup.h"
#include"fm/match_engine/MatchEngineTypes.h"

#include<optional>
#include<vector>

enum class MatchPhase {
    NotStarted,
    FirstHalf,
    HalfTime,
    SecondHalf,
    StoppageTime,
    Finished
};

enum class BallControlState {
    Controlled,
    InFlight,
    Loose,
    OutOfPlay
};

enum class TeamSide {
    Home,
    Away
};

enum class PossessionPhase {
    BuildUp,
    Progression,
    FinalThird,
    Transition,
    SetPiece,
    DefensiveRecovery
};

struct PlayerSimState {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    TeamSide side = TeamSide::Home;

    PitchPoint position;
    PitchPoint targetPosition;

    PlayerIntent currentIntent;

    bool hasBall = false;

    double fatigue = 0.0;
    double effectivePace = 0.0;
    double effectiveAcceleration = 0.0;

    int baseOverall = 0;
};

struct TeamSimState {
    LeagueId leagueId = 0;
    TeamId teamId = 0;
    TeamSide side = TeamSide::Home;

    TacticalSetup tacticalSetup;

    int goals = 0;
    double possessionShareAccumulator = 0.0;

    std::vector<PlayerSimState> players;
};

struct BallState {
    BallControlState controlState = BallControlState::OutOfPlay;
    PitchPoint position;
    PlayerId carrierPlayerId = 0;
    TeamId carrierTeamId = 0;
    std::optional<BallTrajectory> trajectory;
};

struct PossessionState {
    TeamId teamInPossession = 0;
    PlayerId ballCarrierId = 0;
    PossessionPhase phase = PossessionPhase::BuildUp;
    int possessionStartSecond = 0;
    int actionDepth = 0;
    PitchPoint possessionStartPoint;
    PitchPoint lastMeaningfulProgressionPoint;
    int lastMeaningfulProgressionSecond = 0;
    bool isTransition = false;
};

struct MatchSimulationState {
    MatchId matchId = 0;
    LeagueId leagueId = 0;

    MatchPhase phase = MatchPhase::NotStarted;
    int currentSecond = 0;

    TeamSimState homeTeam;
    TeamSimState awayTeam;

    BallState ball;
    PossessionState possession;

    std::vector<MatchTraceFrame> traceFrames;
};

inline TeamSimState* findTeamState(MatchSimulationState& state, TeamId teamId) {
    if (teamId == 0) {
        return nullptr;
    }

    if (state.homeTeam.teamId == teamId) {
        return &state.homeTeam;
    }
    if (state.awayTeam.teamId == teamId) {
        return &state.awayTeam;
    }

    return nullptr;
}

inline const TeamSimState* findTeamState(const MatchSimulationState& state, TeamId teamId) {
    if (teamId == 0) {
        return nullptr;
    }

    if (state.homeTeam.teamId == teamId) {
        return &state.homeTeam;
    }
    if (state.awayTeam.teamId == teamId) {
        return &state.awayTeam;
    }

    return nullptr;
}

inline PlayerSimState* findPlayerState(MatchSimulationState& state, PlayerId playerId) {
    if (playerId == 0) {
        return nullptr;
    }

    for (PlayerSimState& player : state.homeTeam.players) {
        if (player.playerId == playerId) {
            return &player;
        }
    }

    for (PlayerSimState& player : state.awayTeam.players) {
        if (player.playerId == playerId) {
            return &player;
        }
    }

    return nullptr;
}

inline const PlayerSimState* findPlayerState(const MatchSimulationState& state, PlayerId playerId) {
    if (playerId == 0) {
        return nullptr;
    }

    for (const PlayerSimState& player : state.homeTeam.players) {
        if (player.playerId == playerId) {
            return &player;
        }
    }

    for (const PlayerSimState& player : state.awayTeam.players) {
        if (player.playerId == playerId) {
            return &player;
        }
    }

    return nullptr;
}
