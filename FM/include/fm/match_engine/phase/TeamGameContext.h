#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/MatchEngineTypes.h"
#include"fm/match_engine/phase/MatchPhaseTypes.h"

#include<string>

struct TeamGameContext {
    TeamId teamId = 0;
    TeamId possessionTeamId = 0;
    bool hasPossession = false;
    bool possessionRegained = false;
    bool cleanPossessionRegain = false;
    bool possessionStartedFromLooseBall = false;
    bool controlledPossessionEstablished = false;
    MatchTeamPhase currentPhase = MatchTeamPhase::SettledDefense;
    MatchTeamPhase previousPhase = MatchTeamPhase::SettledDefense;
    PitchPoint ballPosition;
    BallZone ballZone = BallZone::MiddleThird;
    BallFlank ballFlank = BallFlank::Center;
    double ballProgress = 0.0;
    bool teamShapeSettled = false;
    bool opponentShapeSettled = false;
    bool supportAvailableNearBall = false;
    double openForwardLaneScore = 0.0;
    double openWideLaneScore = 0.0;
    bool centralSpaceAvailable = false;
    bool wideSpaceAvailableLeft = false;
    bool wideSpaceAvailableRight = false;
    double counterOpportunityScore = 0.0;
    double counterThreatAgainstTeamScore = 0.0;
    bool restDefenseStable = false;
    int playersAheadOfBall = 0;
    int playersBehindBall = 0;
    int nearestSupportCount = 0;
    int possessionActionDepth = 0;
    double secondsInPossession = 0.0;
    double secondsInCurrentPhase = 0.0;
    std::string phaseEntryReason;
    std::string phaseExitReason;
};
