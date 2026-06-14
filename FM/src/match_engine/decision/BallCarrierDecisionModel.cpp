#include"fm/match_engine/decision/BallCarrierDecisionModel.h"

#include"fm/match_engine/decision/ActionScoringModel.h"
#include"fm/match_engine/decision/CarryOptionEvaluator.h"
#include"fm/match_engine/decision/PassOptionEvaluator.h"
#include"fm/match_engine/decision/PhaseAwareDecisionModel.h"
#include"fm/match_engine/decision/ShotDecisionEvaluator.h"

namespace {
    bool isOwnDefensiveDanger(PitchPoint point, AttackingDirection direction) {
        if (direction == AttackingDirection::HomeToAway) {
            return point.x <= 22.0 || PitchGeometry::isInsideHomePenaltyArea(point);
        }

        return point.x >= PitchGeometry::LengthMeters - 22.0
            || PitchGeometry::isInsideAwayPenaltyArea(point);
    }
}

std::vector<ActionCandidate> BallCarrierDecisionModel::evaluate(
    const BallCarrierDecisionModelContext& context) const {
    std::vector<ActionCandidate> candidates;
    if (context.carrierState == nullptr) {
        return candidates;
    }

    PlayerDecisionContext player = context.playerContext;
    player.receivedFinalBall = context.receivedFinalBall;
    player.receivedCutback = context.receivedCutback;
    const ActionScoringModel scoringModel;
    const ActionScoringContext scoringContext{
        player,
        context.attackingDirection
    };

    candidates.push_back(scoringModel.buildHoldCandidate(scoringContext));

    const std::vector<PassOption> passOptions = PassOptionEvaluator{}.evaluate(
        PassOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            player.role,
            player.tacticalSetup,
            player.ballPosition,
            context.attackingDirection,
            player.localPressure
        });
    for (const PassOption& option : passOptions) {
        candidates.push_back(scoringModel.buildPassCandidate(option, scoringContext));
    }

    const std::vector<CarryOption> carryOptions = CarryOptionEvaluator{}.evaluate(
        CarryOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            player.role,
            player.tacticalSetup,
            player.ballPosition,
            context.attackingDirection,
            player.localPressure
        });
    for (const CarryOption& option : carryOptions) {
        candidates.push_back(scoringModel.buildCarryCandidate(option, scoringContext));
    }

    const std::vector<ShotOption> shotOptions = ShotDecisionEvaluator{}.evaluate(
        ShotOptionEvaluationContext{
            context.teamSnapshot,
            context.opponentSnapshot,
            context.teamState,
            context.opponentState,
            context.carrierState,
            player.role,
            player.tacticalSetup,
            player.ballPosition,
            context.attackingDirection,
            player.localPressure,
            player.phase,
            player.possession.possessionActionCount,
            player.possession.ballProgression,
            player.possession.safeCirculationAvailable,
            context.receivedFinalBall,
            context.receivedCutback
        });
    for (const ShotOption& option : shotOptions) {
        candidates.push_back(scoringModel.buildShotCandidate(option, scoringContext));
    }

    if (isOwnDefensiveDanger(player.ballPosition, context.attackingDirection)) {
        candidates.push_back(scoringModel.buildClearCandidate(scoringContext));
    }

    PhaseAwareDecisionModel{}.adjustCandidates(
        PhaseAwareDecisionContext{
            context.teamPhase,
            context.teamSnapshot,
            context.teamGameContext,
            context.carrierGameContext,
            player.role,
            player.ballPosition,
            context.attackingDirection
        },
        candidates);

    return candidates;
}
