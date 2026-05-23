#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/InterceptionResolver.h"
#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>
#include<optional>
#include<vector>

enum class ContestSide {
    None,
    Attacking,
    Defending,
    Neutral
};

enum class ContestResolutionType {
    NoContest,
    AttackerWon,
    DefenderWon,
    KeeperSaved,
    ShotBeatsKeeper,
    BallDeflected,
    BallLoose,
    OutOfPlay
};

struct ContestParticipant {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    ContestSide side = ContestSide::Neutral;

    PitchPoint position;

    PlayerAttributes attributes;
    int baseOverall = 0;

    double fatigue = 0.0;
    double effectivePace = 0.0;
    double effectiveAcceleration = 0.0;

    double arrivalSecond = 0.0;
    double startingAdvantage = 0.0;
};

struct ContestantScore {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    ContestSide side = ContestSide::Neutral;

    double attributeScore = 0.0;
    double timingScore = 0.0;
    double contextScore = 0.0;
    double randomScore = 0.0;
    double finalScore = 0.0;
};

struct ContestResolverRequest {
    ContestType type = ContestType::ReceptionDuel;
    PitchPoint contestPoint;

    std::vector<ContestParticipant> participants;

    std::optional<InterceptionCandidate> interceptionCandidate;

    double ballArrivalSecond = 0.0;
    double pressure = 0.0;
    double executionQuality = 70.0;

    std::uint64_t seed = 0;
};

struct ContestResolverResult {
    ContestType type = ContestType::ReceptionDuel;
    ContestResolutionType resolution = ContestResolutionType::NoContest;

    std::optional<ContestParticipant> winner;
    std::optional<ContestParticipant> loser;

    std::vector<ContestantScore> scores;

    ContestSide winningSide = ContestSide::None;

    bool attackingSideSucceeded = false;
    bool possessionChanges = false;
    bool ballBecomesLoose = false;

    double winningMargin = 0.0;
};

class ContestResolver {
public:
    ContestResolverResult resolve(const ContestResolverRequest& request) const;
};
