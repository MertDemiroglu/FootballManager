#include"fm/match_engine/ball/PassResolutionModel.h"

#include"../DeterministicRandom.h"

#include<algorithm>

namespace {
    struct PassResolutionProfile {
        double routinePassSafetyBonus = 8.0;
        double highPressureThreshold = 36.0;
        double closeDefenderMeters = 3.0;
        double closeLaneMeters = 2.8;
    };

    bool isRoutinePass(BallCarrierActionType type) {
        return type == BallCarrierActionType::ShortPass
            || type == BallCarrierActionType::BackPass
            || type == BallCarrierActionType::SwitchPlay;
    }

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    double unitRoll(std::uint64_t seed, std::uint64_t purpose) {
        return matchEngineDeterministicUnitInterval(
            matchEngineMix64(seed ^ matchEngineMix64(purpose)));
    }

    bool defenderCloseEnoughToContest(const PassResolutionContext& context) {
        const PassResolutionProfile profile;
        const double defenderArrivalMargin =
            std::min(context.defenderArrivalSecond, context.defenderLaneArrivalSecond)
            - context.ballArrivalSecond;
        return context.hasRelevantDefender
            && (context.defenderDistanceToArrival <= profile.closeDefenderMeters
                || context.defenderDistanceToReceiver <= profile.closeDefenderMeters + 1.0
                || context.defenderDistanceToLane <= profile.closeLaneMeters
                || defenderArrivalMargin <= 0.35);
    }

    double receiverSecurity(const PassResolutionContext& context) {
        const double reachSlack =
            context.receiverControlRange - context.receiverDistanceToArrival;
        const double routineBonus =
            isRoutinePass(context.actionType)
                ? PassResolutionProfile{}.routinePassSafetyBonus
                : 0.0;

        return routineBonus
            + (context.executionQuality * 0.34)
            + (reachSlack * 9.0)
            - (context.pressure * 0.14);
    }

    double defenderThreat(const PassResolutionContext& context) {
        if (!context.hasRelevantDefender) {
            return 0.0;
        }

        const double defenderArrivalMargin =
            std::min(context.defenderArrivalSecond, context.defenderLaneArrivalSecond)
            - context.ballArrivalSecond;
        const double timingThreat =
            clampDouble(14.0 - (defenderArrivalMargin * 12.0), 0.0, 22.0);
        const double arrivalThreat =
            clampDouble(24.0 - (context.defenderDistanceToArrival * 5.0), 0.0, 24.0);
        const double receiverThreat =
            clampDouble(20.0 - (context.defenderDistanceToReceiver * 4.0), 0.0, 20.0);
        const double laneThreat =
            clampDouble(18.0 - (context.defenderDistanceToLane * 5.0), 0.0, 18.0);

        return context.defenderIntentPressure
            + arrivalThreat
            + receiverThreat
            + laneThreat
            + timingThreat
            + (context.pressure * 0.13);
    }

    PassResolutionResult resultFor(PassResolutionOutcome outcome) {
        switch (outcome) {
        case PassResolutionOutcome::CleanReceive:
        case PassResolutionOutcome::ContestedReceive:
            return PassResolutionResult{ outcome, true, false, false, false };
        case PassResolutionOutcome::DefenderIntercept:
            return PassResolutionResult{ outcome, false, true, false, true };
        case PassResolutionOutcome::DeflectedLoose:
        case PassResolutionOutcome::MisplacedLoose:
        case PassResolutionOutcome::OutOfPlay:
            return PassResolutionResult{ outcome, false, false, true, false };
        }

        return PassResolutionResult{};
    }
}

PassResolutionResult PassResolutionModel::resolve(const PassResolutionContext& context) const {
    if (context.receiverDistanceToArrival > context.receiverControlRange + 0.35) {
        return resultFor(PassResolutionOutcome::MisplacedLoose);
    }

    const double executionError =
        clampDouble((100.0 - context.executionQuality) * 0.12, 0.0, 16.0);
    const double pressureError =
        clampDouble(context.pressure * 0.10, 0.0, 18.0);
    const double reachSlack =
        context.receiverControlRange - context.receiverDistanceToArrival;
    const double misplacedChance =
        clampDouble(executionError + pressureError - (reachSlack * 7.0), 0.0, 38.0);
    if (unitRoll(context.seed, 0x7f96a33e1f2d71ULL) * 100.0 < misplacedChance) {
        return resultFor(PassResolutionOutcome::MisplacedLoose);
    }

    if (!defenderCloseEnoughToContest(context)) {
        return context.pressure >= PassResolutionProfile{}.highPressureThreshold
            ? resultFor(PassResolutionOutcome::ContestedReceive)
            : resultFor(PassResolutionOutcome::CleanReceive);
    }

    const double contestBalance = defenderThreat(context) - receiverSecurity(context);
    const double interceptChance =
        clampDouble((contestBalance * 1.15) + (context.pressure * 0.08), 0.0, 48.0);
    const double deflectChance =
        clampDouble(10.0 + (contestBalance * 0.75) + (context.pressure * 0.09), 0.0, 42.0);
    const double roll = unitRoll(context.seed, 0xbe74fd3592b5c9ULL) * 100.0;

    if (roll < interceptChance) {
        return resultFor(PassResolutionOutcome::DefenderIntercept);
    }
    if (roll < interceptChance + deflectChance) {
        return resultFor(PassResolutionOutcome::DeflectedLoose);
    }

    return contestBalance > -6.0
        ? resultFor(PassResolutionOutcome::ContestedReceive)
        : resultFor(PassResolutionOutcome::CleanReceive);
}
