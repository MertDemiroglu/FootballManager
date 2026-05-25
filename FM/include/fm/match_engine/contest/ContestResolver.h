#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/contest/ContestTypes.h"
#include"fm/match_engine/contest/InterceptionResolver.h"
#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/roster/PlayerAttributes.h"

#include<cstdint>
#include<optional>
#include<vector>

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
    double finalScore = 0.0;
    double selectionWeight = 0.0;
};

struct ContestResolverRequest {
    ContestType type = ContestType::ReceptionDuel;
    PitchPoint contestPoint;

    std::vector<ContestParticipant> participants;

    std::optional<InterceptionCandidate> interceptionCandidate;

    double ballArrivalSecond = 0.0;
    double pressure = 0.0; // 0-100
    double executionQuality = 70.0;

    std::uint64_t seed = 0;
};

struct ContestResolverResult {
    ContestType type = ContestType::ReceptionDuel;
    ContestResolutionType resolution = ContestResolutionType::NoContest;
    ContestBallOutcome ballOutcome = ContestBallOutcome::None;

    // Contest/action winner, not necessarily the clean post-contest ball controller.
    std::optional<ContestParticipant> winner;
    std::optional<ContestParticipant> loser;
    // Set only when a player actually controls the ball after the contest.
    std::optional<ContestParticipant> cleanController;

    std::vector<ContestantScore> scores;

    ContestSide winningSide = ContestSide::None;

    bool attackingSideSucceeded = false;
    bool defendingActionSucceeded = false;
    bool possessionChanges = false;
    bool ballBecomesLoose = false;
    bool ballDeflected = false;
    bool cleanPossessionWon = false;

    double winningMargin = 0.0;
};

class ContestResolver {
public:
    ContestResolverResult resolve(const ContestResolverRequest& request) const;
};
