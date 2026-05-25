#include"fm/match_engine/movement/TeamShapeModel.h"

#include"fm/roster/Formation.h"
#include"fm/roster/FormationPitchLayout.h"

#include<algorithm>
#include<stdexcept>
#include<string>
#include<unordered_set>

namespace {
    double attackSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    bool isWideRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            return true;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::Striker:
            return false;
        }

        return false;
    }

    bool isDefensiveLineRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
        case FormationSlotRole::DefensiveMidfielder:
            return true;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
        case FormationSlotRole::Striker:
            return false;
        }

        return false;
    }

    PitchPoint toPitchPoint(FormationPitchCoordinate coordinate, AttackingDirection direction) {
        const double lateral = std::clamp(coordinate.x, 0.0, 1.0) * PitchGeometry::WidthMeters;
        const double progressFromOwnGoal = (1.0 - std::clamp(coordinate.y, 0.0, 1.0))
            * PitchGeometry::LengthMeters;

        if (direction == AttackingDirection::HomeToAway) {
            return PitchGeometry::clampToPitch(PitchPoint{ progressFromOwnGoal, lateral });
        }

        return PitchGeometry::clampToPitch(PitchPoint{
            PitchGeometry::LengthMeters - progressFromOwnGoal,
            PitchGeometry::WidthMeters - lateral
        });
    }

    PitchPoint basePositionForSlot(
        const TeamSheet& teamSheet,
        std::size_t slotIndex,
        FormationSlotRole slotRole,
        AttackingDirection direction) {
        const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(teamSheet.formation);
        if (slotIndex >= slotTemplate.size()) {
            throw std::runtime_error("team shape model received an assignment with an invalid slot index");
        }
        if (slotTemplate[slotIndex] != slotRole) {
            throw std::runtime_error("team shape model received an assignment whose slot role does not match formation");
        }

        const FormationPitchCoordinate coordinate = getFormationPitchCoordinate(
            teamSheet.formation,
            slotIndex,
            slotRole).value_or(getFallbackFormationPitchCoordinate(slotIndex, slotTemplate.size(), slotRole));

        return toPitchPoint(coordinate, direction);
    }

    PitchPoint applyTacticalAdjustments(
        PitchPoint basePosition,
        FormationSlotRole slotRole,
        const TeamShapeContext& context) {
        PitchPoint adjusted = basePosition;
        const double direction = attackSign(context.attackingDirection);

        switch (context.tacticalSetup.mentality) {
        case TeamMentality::Defensive:
            adjusted.x -= direction * 3.0;
            break;
        case TeamMentality::Balanced:
            break;
        case TeamMentality::Attacking:
            adjusted.x += direction * 3.0;
            break;
        }

        if (isWideRole(slotRole)) {
            const double centerY = PitchGeometry::WidthMeters / 2.0;
            const double fromCenter = adjusted.y - centerY;
            switch (context.tacticalSetup.width) {
            case TeamWidth::Narrow:
                adjusted.y -= fromCenter == 0.0 ? 0.0 : (fromCenter > 0.0 ? 4.0 : -4.0);
                break;
            case TeamWidth::Balanced:
                break;
            case TeamWidth::Wide:
                adjusted.y += fromCenter == 0.0 ? 0.0 : (fromCenter > 0.0 ? 4.0 : -4.0);
                break;
            }
        }

        if (isDefensiveLineRole(slotRole)) {
            switch (context.tacticalSetup.defensiveLine) {
            case DefensiveLine::Low:
                adjusted.x -= direction * 4.0;
                break;
            case DefensiveLine::Standard:
                break;
            case DefensiveLine::High:
                adjusted.x += direction * 4.0;
                break;
            }
        }

        // PressingIntensity and MarkingStyle will affect local intents such as
        // PressBallCarrier, MarkOpponent, CoverSpace, and InterceptBallPath.
        // PassingDirectness will affect future ActionCandidate scoring.
        return PitchGeometry::clampToPitch(adjusted);
    }

    void appendTargetForAssignment(
        std::vector<PlayerShapeTarget>& targets,
        std::unordered_set<PlayerId>& seenPlayers,
        const TeamShapeContext& context,
        const TeamSheet& teamSheet,
        std::size_t slotIndex,
        FormationSlotRole slotRole,
        PlayerId playerId) {
        if (playerId == 0) {
            return;
        }

        if (!seenPlayers.insert(playerId).second) {
            throw std::runtime_error("team shape model cannot build duplicate starter targets");
        }

        const PitchPoint basePosition = basePositionForSlot(
            teamSheet,
            slotIndex,
            slotRole,
            context.attackingDirection);
        const PitchPoint tacticalPosition = applyTacticalAdjustments(basePosition, slotRole, context);

        targets.push_back(PlayerShapeTarget{
            playerId,
            context.teamId,
            static_cast<int>(slotIndex),
            slotRole,
            basePosition,
            tacticalPosition,
            tacticalPosition
        });
    }
}

AttackingDirection attackingDirectionFor(bool isHomeTeam) {
    return isHomeTeam ? AttackingDirection::HomeToAway : AttackingDirection::AwayToHome;
}

std::vector<PlayerShapeTarget> TeamShapeModel::buildTargets(
    const TeamShapeContext& context,
    const TeamSheet& teamSheet) const {
    std::vector<PlayerShapeTarget> targets;
    targets.reserve(teamSheet.startingAssignments.empty()
        ? teamSheet.startingPlayerIds.size()
        : teamSheet.startingAssignments.size());

    std::unordered_set<PlayerId> seenPlayers;
    seenPlayers.reserve(targets.capacity());

    if (!teamSheet.startingAssignments.empty()) {
        for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
            appendTargetForAssignment(
                targets,
                seenPlayers,
                context,
                teamSheet,
                assignment.slotIndex,
                assignment.slotRole,
                assignment.playerId);
        }
        return targets;
    }

    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(teamSheet.formation);
    for (std::size_t i = 0; i < teamSheet.startingPlayerIds.size() && i < slotTemplate.size(); ++i) {
        appendTargetForAssignment(
            targets,
            seenPlayers,
            context,
            teamSheet,
            i,
            slotTemplate[i],
            teamSheet.startingPlayerIds[i]);
    }

    return targets;
}
