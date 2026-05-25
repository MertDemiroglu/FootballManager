#include"fm/match_engine/decision/ActionCandidateGenerator.h"

#include"fm/match_engine/decision/BallCarrierDecisionModel.h"

#include<algorithm>
#include<cmath>
#include<limits>
#include<vector>

namespace {
    double clampScore(double value) {
        return std::clamp(value, 0.0, 100.0);
    }

    double directionSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    PitchPoint advanceTarget(PitchPoint position, AttackingDirection direction, double meters) {
        return PitchGeometry::clampToPitch(PitchPoint{
            position.x + directionSign(direction) * meters,
            position.y
        });
    }

    bool isOwnDefensiveDanger(PitchPoint point, AttackingDirection direction) {
        if (direction == AttackingDirection::HomeToAway) {
            return point.x <= 22.0 || PitchGeometry::isInsideHomePenaltyArea(point);
        }

        return point.x >= PitchGeometry::LengthMeters - 22.0
            || PitchGeometry::isInsideAwayPenaltyArea(point);
    }

    double nearestOpponentPressure(
        PitchPoint position,
        const std::vector<PlayerSimState>& opponents) {
        double nearest = std::numeric_limits<double>::max();
        for (const PlayerSimState& opponent : opponents) {
            nearest = std::min(nearest, PitchGeometry::distance(position, opponent.position));
        }

        if (nearest <= 4.0) {
            return 35.0;
        }
        if (nearest <= 8.0) {
            return 22.0;
        }
        if (nearest <= 14.0) {
            return 10.0;
        }

        return opponents.empty() ? 0.0 : 3.0;
    }

    FormationSlotRole assignedRoleFor(const MatchTeamSnapshot* teamSnapshot, PlayerId playerId) {
        if (teamSnapshot == nullptr) {
            return FormationSlotRole::Unknown;
        }

        for (const TeamSheetSlotAssignment& assignment : teamSnapshot->teamSheet.startingAssignments) {
            if (assignment.playerId == playerId) {
                return assignment.slotRole;
            }
        }

        return FormationSlotRole::Unknown;
    }
    double mentalitySafeBonus(TeamMentality mentality) {
        return mentality == TeamMentality::Defensive ? 10.0 : 0.0;
    }

    ActionCandidate buildCandidate(
        BallCarrierActionType type,
        PitchPoint target,
        PlayerId targetPlayerId,
        double tacticalScore,
        double contextScore,
        double skillConfidenceScore,
        double pressurePenalty) {
        ActionCandidate candidate;
        candidate.type = type;
        candidate.intendedTarget = PitchGeometry::clampToPitch(target);
        candidate.targetPlayerId = targetPlayerId;
        candidate.tacticalScore = tacticalScore;
        candidate.contextScore = contextScore;
        candidate.mentalScore = 0.0;
        candidate.skillConfidenceScore = skillConfidenceScore;
        candidate.pressurePenalty = pressurePenalty;
        candidate.finalScore = clampScore(
            tacticalScore + contextScore + skillConfidenceScore - pressurePenalty);
        return candidate;
    }

}

std::vector<ActionCandidate> ActionCandidateGenerator::generate(
    const ActionCandidateGenerationRequest& request) const {
    std::vector<ActionCandidate> candidates;

    const TacticalSetup& tacticalSetup = request.teamShapeContext.tacticalSetup;
    const AttackingDirection direction = request.teamShapeContext.attackingDirection;
    const PitchPoint carrierPosition = request.ballCarrier.position;
    const double pressurePenalty = nearestOpponentPressure(carrierPosition, request.opponents);
    const TeamSimState* requestTeamState =
        findTeamState(request.simulationState, request.ballCarrier.teamId);
    const TeamSimState* requestOpponentState =
        request.simulationState.homeTeam.teamId == request.ballCarrier.teamId
            ? &request.simulationState.awayTeam
            : &request.simulationState.homeTeam;
    const bool controlledBall =
        request.ballState.controlState == BallControlState::Controlled
        && request.ballState.carrierPlayerId == request.ballCarrier.playerId;
    const FormationSlotRole carrierRole = assignedRoleFor(
        request.teamSnapshot,
        request.ballCarrier.playerId);

    candidates.push_back(buildCandidate(
        BallCarrierActionType::Hold,
        carrierPosition,
        0,
        18.0 + mentalitySafeBonus(tacticalSetup.mentality) * 0.5,
        controlledBall ? 12.0 : 4.0,
        8.0,
        pressurePenalty * 0.45));

    const std::vector<ActionCandidate> ballCarrierOptions = BallCarrierDecisionModel{}.evaluate(
        BallCarrierDecisionModelContext{
            request.teamSnapshot,
            request.opponentSnapshot,
            requestTeamState,
            requestOpponentState,
            &request.ballCarrier,
            carrierRole,
            tacticalSetup,
            carrierPosition,
            direction,
            pressurePenalty,
            request.simulationState.possession.actionDepth,
            request.simulationState.possession.isTransition
        });
    candidates.insert(candidates.end(), ballCarrierOptions.begin(), ballCarrierOptions.end());

    if (isOwnDefensiveDanger(carrierPosition, direction)) {
        candidates.push_back(buildCandidate(
            BallCarrierActionType::Clear,
            advanceTarget(carrierPosition, direction, 35.0),
            0,
            32.0 + mentalitySafeBonus(tacticalSetup.mentality),
            24.0,
            8.0,
            pressurePenalty * 0.2));
    }

    return candidates;
}
