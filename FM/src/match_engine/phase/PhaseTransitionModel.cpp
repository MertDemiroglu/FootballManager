#include"fm/match_engine/phase/PhaseTransitionModel.h"

#include<algorithm>

namespace {
    bool canLeavePhase(const TeamGameContext& context, const PhaseTransitionConfig& config) {
        return context.secondsInCurrentPhase >= config.minimumPhaseDurationSeconds
            || context.possessionRegained
            || context.hasPossession != isInPossessionPhase(context.currentPhase);
    }

    PhaseTransitionResult stay(const TeamGameContext& context) {
        return PhaseTransitionResult{
            context.currentPhase,
            false,
            context.phaseEntryReason,
            ""
        };
    }

    PhaseTransitionResult changeTo(
        MatchTeamPhase phase,
        const char* entryReason,
        const char* exitReason) {
        return PhaseTransitionResult{
            phase,
            true,
            entryReason,
            exitReason
        };
    }
}

PhaseTransitionModel::PhaseTransitionModel(PhaseTransitionConfig config)
    : config_(config) {
}

PhaseTransitionResult PhaseTransitionModel::evaluate(const TeamGameContext& context) const {
    if (context.possessionTeamId == 0) {
        return stay(context);
    }

    if (!context.hasPossession) {
        const bool immediateCounterThreat =
            context.counterThreatAgainstTeamScore >= config_.defenseRecoveredThreatThreshold
            || (!context.restDefenseStable && context.playersAheadOfBall >= 4);

        if (isInPossessionPhase(context.currentPhase)) {
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
    const bool plausibleCounter =
        context.possessionRegained
        && !context.opponentShapeSettled
        && context.openForwardLaneScore >= config_.counterForwardLaneThreshold
        && (context.playersAheadOfBall > 0 || context.centralSpaceAvailable);
    if (plausibleCounter) {
        return changeTo(
            MatchTeamPhase::CounterAttack,
            "regain with unsettled opponent and forward lane",
            currentPhaseIsDefensive ? "possession regained" : "counter opportunity opened");
    }

    if (currentPhaseIsDefensive) {
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

    if (!canLeavePhase(context, config_)) {
        return stay(context);
    }

    if (context.currentPhase == MatchTeamPhase::BuildUp) {
        if (context.ballProgress >= config_.finalizingEnterProgress
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
