#include"fm/match_engine/phase/PhaseTransitionModel.h"

#include<algorithm>

namespace {
    bool controlledPossessionEstablished(const TeamGameContext& context, const PhaseTransitionConfig& config) {
        return context.controlledPossessionEstablished
            || context.possessionActionDepth >= config.minimumControlledPossessionActions
            || context.secondsInPossession >= config.minimumControlledPossessionSeconds;
    }

    bool canLeavePhase(const TeamGameContext& context, const PhaseTransitionConfig& config) {
        return context.secondsInCurrentPhase >= config.minimumPhaseDurationSeconds
            && controlledPossessionEstablished(context, config);
    }

    PhaseTransitionResult stay(const TeamGameContext& context) {
        return PhaseTransitionResult{
            context.currentPhase,
            false,
            false,
            CounterRejectionReason::None,
            context.phaseEntryReason,
            ""
        };
    }

    PhaseTransitionResult looseHold(const TeamGameContext& context) {
        PhaseTransitionResult result = stay(context);
        result.heldForLooseBall = true;
        return result;
    }

    PhaseTransitionResult counterRejected(
        const TeamGameContext& context,
        CounterRejectionReason reason) {
        PhaseTransitionResult result = stay(context);
        result.counterRejection = reason;
        return result;
    }

    PhaseTransitionResult changeTo(
        MatchTeamPhase phase,
        const char* entryReason,
        const char* exitReason) {
        return PhaseTransitionResult{
            phase,
            true,
            false,
            CounterRejectionReason::None,
            entryReason,
            exitReason
        };
    }

    PhaseTransitionResult counterRejectedBuildUp(
        CounterRejectionReason reason,
        const char* entryReason,
        const char* exitReason) {
        PhaseTransitionResult result = changeTo(MatchTeamPhase::BuildUp, entryReason, exitReason);
        result.counterRejection = reason;
        return result;
    }
}

PhaseTransitionModel::PhaseTransitionModel(PhaseTransitionConfig config)
    : config_(config) {
}

PhaseTransitionResult PhaseTransitionModel::evaluate(const TeamGameContext& context) const {
    if (context.possessionTeamId == 0) {
        return looseHold(context);
    }

    if (!context.hasPossession) {
        if (isInPossessionPhase(context.currentPhase)
            && !controlledPossessionEstablished(context, config_)) {
            return stay(context);
        }

        const bool immediateCounterThreat =
            context.counterThreatAgainstTeamScore >= config_.defensiveTransitionThreatThreshold
            && (!context.restDefenseStable
                || context.playersAheadOfBall >= config_.defensiveTransitionExposedPlayersAhead);

        if (isInPossessionPhase(context.currentPhase)) {
            if (context.secondsInCurrentPhase < config_.minimumPhaseDurationSeconds
                && !immediateCounterThreat) {
                return stay(context);
            }
            if (context.restDefenseStable && !immediateCounterThreat) {
                return changeTo(
                    MatchTeamPhase::SettledDefense,
                    "possession lost with compact rest defense",
                    "lost possession");
            }
            return changeTo(
                MatchTeamPhase::DefensiveTransition,
                "possession lost with exposed rest defense",
                "lost possession");
        }

        if (context.currentPhase == MatchTeamPhase::DefensiveTransition) {
            if (!canLeavePhase(context, config_)) {
                return stay(context);
            }
            if ((context.playersBehindBall >= config_.defenseRecoveredPlayersBehindBall
                    && context.counterThreatAgainstTeamScore < config_.defenseRecoveredThreatThreshold)
                || context.restDefenseStable) {
                return changeTo(
                    MatchTeamPhase::SettledDefense,
                    "defensive shape recovered",
                    "defense recovered");
            }
        }

        if (context.currentPhase == MatchTeamPhase::SettledDefense
            && canLeavePhase(context, config_)
            && !context.restDefenseStable
            && context.counterThreatAgainstTeamScore >= config_.settledDefenseBreakThreatThreshold) {
            return changeTo(
                MatchTeamPhase::DefensiveTransition,
                "opponent broke defensive shape quickly",
                "shape broken");
        }

        return stay(context);
    }

    const bool currentPhaseIsDefensive = !isInPossessionPhase(context.currentPhase);
    const bool settledPossessionRecycling =
        context.possessionActionDepth > config_.minimumControlledPossessionActions
        || context.secondsInPossession > config_.minimumControlledPossessionSeconds * 2.0;
    if (context.possessionRegained) {
        const auto rejectCounter = [&](CounterRejectionReason reason) {
            if (currentPhaseIsDefensive && controlledPossessionEstablished(context, config_)) {
                return counterRejectedBuildUp(
                    reason,
                    "controlled possession established after rejected counter",
                    "possession regained without valid counter");
            }
            return counterRejected(context, reason);
        };

        if (!context.cleanPossessionRegain || context.possessionStartedFromLooseBall) {
            return rejectCounter(CounterRejectionReason::NoCleanRegain);
        }
        if (settledPossessionRecycling) {
            return rejectCounter(CounterRejectionReason::SettledPossession);
        }
        if (context.opponentShapeSettled) {
            return rejectCounter(CounterRejectionReason::OpponentRecovered);
        }
        if (context.ballProgress < config_.counterMinimumBallProgress
            || context.openForwardLaneScore < config_.counterForwardLaneThreshold
            || context.counterOpportunityScore < config_.counterForwardLaneThreshold) {
            return rejectCounter(CounterRejectionReason::NoForwardLane);
        }
        if (context.playersAheadOfBall < config_.counterMinimumRunnersAhead) {
            return rejectCounter(CounterRejectionReason::NoRunner);
        }
        return changeTo(
            MatchTeamPhase::CounterAttack,
            "regain with unsettled opponent and forward lane",
            currentPhaseIsDefensive ? "possession regained" : "counter opportunity opened");
    }

    if (currentPhaseIsDefensive) {
        if (!controlledPossessionEstablished(context, config_)) {
            return stay(context);
        }
        if (context.secondsInCurrentPhase < config_.minimumPhaseDurationSeconds) {
            return stay(context);
        }
        return changeTo(
            MatchTeamPhase::BuildUp,
            "controlled possession established",
            "possession regained");
    }

    if (context.currentPhase == MatchTeamPhase::CounterAttack) {
        if (context.secondsInCurrentPhase < config_.minimumCounterDurationSeconds) {
            return stay(context);
        }
        if (context.ballProgress >= config_.finalizingEnterProgress
            && (context.nearestSupportCount >= 1 || context.supportAvailableNearBall)) {
            return changeTo(
                MatchTeamPhase::FinalizingPosition,
                "counter reached attacking territory",
                "attack slowed into chance creation");
        }
        if (context.openForwardLaneScore < config_.counterExpireForwardLaneThreshold) {
            return changeTo(
                MatchTeamPhase::BuildUp,
                "forward lane closed",
                "counter expired: no forward lane");
        }
        if (context.opponentShapeSettled) {
            return changeTo(
                MatchTeamPhase::BuildUp,
                "opponent recovered shape",
                "counter expired: defense recovered");
        }
        if (context.possessionActionDepth >= 4 && context.ballProgress < config_.finalizingEnterProgress) {
            return changeTo(
                MatchTeamPhase::BuildUp,
                "attack recycled before finalizing territory",
                "counter expired: recycled to buildup");
        }
        return stay(context);
    }

    if (!controlledPossessionEstablished(context, config_)) {
        return stay(context);
    }

    if (!canLeavePhase(context, config_)) {
        return stay(context);
    }

    if (context.currentPhase == MatchTeamPhase::BuildUp) {
        if (context.ballProgress >= config_.finalizingEnterProgress
            && context.possessionActionDepth >= config_.minimumFinalizingPossessionActions
            && context.nearestSupportCount >= config_.finalizingSupportCount
            && (context.teamShapeSettled || context.supportAvailableNearBall)) {
            return changeTo(
                MatchTeamPhase::FinalizingPosition,
                "controlled progression reached attacking territory",
                "buildup completed");
        }
    } else if (context.currentPhase == MatchTeamPhase::FinalizingPosition) {
        if (context.ballProgress <= config_.finalizingExitProgress) {
            return changeTo(
                MatchTeamPhase::BuildUp,
                "ball recycled into deeper territory",
                "forced backward or recycled");
        }
        if (context.openForwardLaneScore < config_.counterExpireForwardLaneThreshold
            && context.opponentShapeSettled
            && context.possessionActionDepth >= 6) {
            return changeTo(
                MatchTeamPhase::BuildUp,
                "settled possession recycled after lanes closed",
                "forward lanes closed");
        }
    }

    return stay(context);
}
