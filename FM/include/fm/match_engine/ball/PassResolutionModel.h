#pragma once

#include"fm/match_engine/MatchEngineTypes.h"

#include<cstdint>

enum class PassResolutionOutcome {
    CleanReceive,
    ContestedReceive,
    DefenderIntercept,
    DeflectedLoose,
    MisplacedLoose,
    OutOfPlay
};

struct PassResolutionContext {
    BallCarrierActionType actionType = BallCarrierActionType::ShortPass;
    PitchPoint startPoint;
    PitchPoint intendedTarget;
    PitchPoint actualTarget;
    double passDistance = 0.0;
    double executionQuality = 70.0;
    double pressure = 0.0;
    double ballArrivalSecond = 0.0;

    double receiverDistanceToArrival = 0.0;
    double receiverControlRange = 1.0;
    double receiverArrivalSecond = 0.0;

    bool hasRelevantDefender = false;
    double defenderDistanceToArrival = 100.0;
    double defenderDistanceToReceiver = 100.0;
    double defenderDistanceToLane = 100.0;
    double defenderArrivalSecond = 100.0;
    double defenderLaneArrivalSecond = 100.0;
    double defenderIntentPressure = 0.0;

    std::uint64_t seed = 0;
};

struct PassResolutionResult {
    PassResolutionOutcome outcome = PassResolutionOutcome::CleanReceive;
    bool completedPass = true;
    bool turnover = false;
    bool looseBall = false;
    bool interception = false;
};

class PassResolutionModel {
public:
    PassResolutionResult resolve(const PassResolutionContext& context) const;
};
