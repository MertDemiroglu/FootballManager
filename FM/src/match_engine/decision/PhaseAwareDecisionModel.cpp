#include"fm/match_engine/decision/PhaseAwareDecisionModel.h"

#include"fm/match_engine/decision/PhaseDecisionContext.h"
#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>

namespace {
    double directionSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    double forwardMeters(PitchPoint from, PitchPoint to, AttackingDirection direction) {
        return (to.x - from.x) * directionSign(direction);
    }

    double attackingProgress(PitchPoint point, AttackingDirection direction) {
        const PitchPoint clamped = PitchGeometry::clampToPitch(point);
        return direction == AttackingDirection::HomeToAway
            ? clamped.x / PitchGeometry::LengthMeters
            : (PitchGeometry::LengthMeters - clamped.x) / PitchGeometry::LengthMeters;
    }

    bool isPassLike(BallCarrierActionType type) {
        return type == BallCarrierActionType::ShortPass
            || type == BallCarrierActionType::BackPass
            || type == BallCarrierActionType::ThroughBall
            || type == BallCarrierActionType::SwitchPlay
            || type == BallCarrierActionType::LowCross
            || type == BallCarrierActionType::HighCross
            || type == BallCarrierActionType::Cutback;
    }

    bool isCarryLike(BallCarrierActionType type) {
        return type == BallCarrierActionType::Carry
            || type == BallCarrierActionType::Dribble
            || type == BallCarrierActionType::CutInside;
    }

    bool isFinalBall(BallCarrierActionType type) {
        return type == BallCarrierActionType::ThroughBall
            || type == BallCarrierActionType::LowCross
            || type == BallCarrierActionType::HighCross
            || type == BallCarrierActionType::Cutback;
    }

    bool isFoundationRole(FormationSlotRole role) {
        const PhaseDecisionRoleBucket bucket = phaseDecisionRoleBucket(role);
        return bucket == PhaseDecisionRoleBucket::CenterBack
            || bucket == PhaseDecisionRoleBucket::Fullback
            || bucket == PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder;
    }

    bool isSupportTargetRole(FormationSlotRole role) {
        const PhaseDecisionRoleBucket bucket = phaseDecisionRoleBucket(role);
        return bucket == PhaseDecisionRoleBucket::Fullback
            || bucket == PhaseDecisionRoleBucket::DefensiveOrCentralMidfielder
            || bucket == PhaseDecisionRoleBucket::Winger;
    }

    FormationSlotRole targetRoleFor(
        const PhaseAwareDecisionContext& context,
        const ActionCandidate& candidate) {
        if (candidate.targetPlayerId == 0 || context.teamSnapshot == nullptr) {
            return FormationSlotRole::Unknown;
        }

        for (const TeamSheetSlotAssignment& assignment : context.teamSnapshot->teamSheet.startingAssignments) {
            if (assignment.playerId == candidate.targetPlayerId) {
                return assignment.slotRole;
            }
        }
        return FormationSlotRole::Unknown;
    }

    double roleMultiplierForBuildUpTarget(
        FormationSlotRole role,
        const PhaseDecisionTuning& tuning) {
        if (isFoundationRole(role)) {
            return tuning.buildUpFoundationTargetMultiplier;
        }
        if (role == FormationSlotRole::Striker) {
            return tuning.buildUpStrikerTargetMultiplier;
        }
        return 1.0;
    }

    double clampScore(double score, const PhaseDecisionTuning& tuning) {
        return std::clamp(score, tuning.minimumSelectionScore, tuning.maximumSelectionScore);
    }

    void applyMultiplier(ActionCandidate& candidate, double multiplier, const PhaseDecisionTuning& tuning) {
        candidate.finalScore = clampScore(candidate.finalScore * multiplier, tuning);
        candidate.contextScore *= multiplier;
    }

    bool finalActionIsGenuinelyAdvanced(
        const PhaseAwareDecisionContext& context,
        const PhaseDecisionTuning& tuning) {
        const double progress = attackingProgress(context.ballPosition, context.attackingDirection);
        const bool wide =
            context.teamGameContext != nullptr
            && context.teamGameContext->ballFlank != BallFlank::Center;
        return progress >= tuning.advancedWideFinalActionProgress && wide;
    }
}

void PhaseAwareDecisionModel::adjustCandidates(
    const PhaseAwareDecisionContext& context,
    std::vector<ActionCandidate>& candidates) const {
    for (ActionCandidate& candidate : candidates) {
        const double forward = forwardMeters(
            context.ballPosition,
            candidate.intendedTarget,
            context.attackingDirection);
        const bool progressivePass =
            isPassLike(candidate.type) && forward >= tuning_.progressivePassMeters;
        const bool progressiveCarry =
            isCarryLike(candidate.type) && forward >= tuning_.progressiveCarryMeters;
        const bool recycle =
            candidate.type == BallCarrierActionType::BackPass
            || (isPassLike(candidate.type) && forward <= tuning_.recycleBackwardMeters);
        const bool forwardLaneOpen =
            context.teamGameContext != nullptr
            && context.teamGameContext->openForwardLaneScore >= tuning_.counterForwardLaneThreshold;

        if (!isInPossessionPhase(context.phase)) {
            applyMultiplier(candidate, tuning_.defensivePhaseActionMultiplier, tuning_);
            continue;
        }

        if (context.phase == MatchTeamPhase::BuildUp) {
            if (isPassLike(candidate.type)) {
                const FormationSlotRole targetRole = targetRoleFor(context, candidate);
                double multiplier = candidate.type == BallCarrierActionType::BackPass
                    ? tuning_.buildUpRecycleMultiplier
                    : tuning_.buildUpSafePassMultiplier;
                multiplier *= roleMultiplierForBuildUpTarget(targetRole, tuning_);
                if (progressivePass && !isFinalBall(candidate.type)) {
                    multiplier *= tuning_.buildUpProgressivePassMultiplier;
                }
                if (isFinalBall(candidate.type)) {
                    multiplier *= finalActionIsGenuinelyAdvanced(context, tuning_)
                        ? tuning_.buildUpAdvancedFinalBallMultiplier
                        : tuning_.buildUpFinalBallMultiplier;
                }
                applyMultiplier(candidate, multiplier, tuning_);
                continue;
            }

            if (isCarryLike(candidate.type)) {
                applyMultiplier(
                    candidate,
                    progressiveCarry ? tuning_.buildUpCarryMultiplier : tuning_.buildUpRiskCarryMultiplier,
                    tuning_);
                continue;
            }

            if (candidate.type == BallCarrierActionType::Shoot) {
                const double multiplier = candidate.finalScore >= tuning_.exceptionalShotScore
                    ? tuning_.buildUpExceptionalShotMultiplier
                    : tuning_.buildUpShotMultiplier;
                applyMultiplier(candidate, multiplier, tuning_);
                continue;
            }
        }

        if (context.phase == MatchTeamPhase::FinalizingPosition) {
            const FormationSlotRole targetRole = targetRoleFor(context, candidate);
            if (isFinalBall(candidate.type)) {
                applyMultiplier(candidate, tuning_.finalizingFinalBallMultiplier, tuning_);
            } else if (candidate.type == BallCarrierActionType::Shoot) {
                const PhaseDecisionRoleBucket carrierBucket =
                    phaseDecisionRoleBucket(context.carrierRole);
                applyMultiplier(
                    candidate,
                    carrierBucket == PhaseDecisionRoleBucket::Striker
                        ? tuning_.finalizingShotMultiplier
                        : tuning_.finalizingSupportRoleShotMultiplier,
                    tuning_);
            } else if (recycle) {
                applyMultiplier(candidate, tuning_.finalizingRecycleMultiplier, tuning_);
            } else if (isPassLike(candidate.type) && isSupportTargetRole(targetRole)) {
                applyMultiplier(candidate, tuning_.finalizingSupportTargetMultiplier, tuning_);
            }
            continue;
        }

        if (context.phase == MatchTeamPhase::CounterAttack) {
            if (candidate.type == BallCarrierActionType::ThroughBall) {
                applyMultiplier(candidate, tuning_.counterThroughBallMultiplier, tuning_);
            } else if (isPassLike(candidate.type) && forward >= tuning_.progressivePassMeters) {
                applyMultiplier(candidate, tuning_.counterForwardPassMultiplier, tuning_);
            } else if (isCarryLike(candidate.type) && forward >= tuning_.progressiveCarryMeters) {
                applyMultiplier(candidate, tuning_.counterForwardCarryMultiplier, tuning_);
            } else if (isFinalBall(candidate.type) || candidate.type == BallCarrierActionType::Shoot) {
                applyMultiplier(candidate, tuning_.counterCreateActionMultiplier, tuning_);
            } else if (recycle) {
                applyMultiplier(
                    candidate,
                    forwardLaneOpen
                        ? tuning_.counterRecycleWithLaneMultiplier
                        : tuning_.counterRecycleNoLaneMultiplier,
                    tuning_);
            }
        }
    }
}
