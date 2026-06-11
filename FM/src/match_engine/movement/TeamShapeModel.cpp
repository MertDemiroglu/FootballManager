#include"fm/match_engine/movement/TeamShapeModel.h"

#include"fm/roster/Formation.h"
#include"fm/roster/FormationPitchLayout.h"

#include<algorithm>
#include<stdexcept>
#include<string>
#include<unordered_set>

namespace {
    struct MentalityShapeProfile {
        double defensiveRecoveryMeters = 0.0;
        double counterPressMultiplier = 1.0;
        double restDefenseGapMeters = 0.0;
    };

    struct RoleShapeProfile {
        double lineWeight = 0.0;
        double ballSideShiftWeight = 0.0;
        double widthHoldWeight = 0.0;
        double counterPressWeight = 0.0;
        double restDefenseGapBonusMeters = 0.0;
    };

    struct DynamicShapeProfile {
        double possessionBallShiftMeters = 0.20;
        double possessionFarSideTuckMeters = 0.25;
        double defensiveBallShiftMeters = 0.40;
        double defensiveCompactTuckMeters = 0.35;
        double transitionCounterPressBlend = 0.08;
        double counterPressProximityMeters = 28.0;
        double goalkeeperDefensiveShiftMeters = 0.10;
        double advancedCentralMinimumGoalDistance = 9.0;
        double advancedWideMinimumGoalDistance = 6.5;
        double supportCentralMinimumGoalDistance = 12.0;
        double supportWideMinimumGoalDistance = 8.5;
        double restDefenseMinimumGoalDistance = 22.0;
    };

    constexpr DynamicShapeProfile kDynamicShapeProfile;

    double attackSign(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway ? 1.0 : -1.0;
    }

    double signedProgressX(PitchPoint point, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? point.x
            : PitchGeometry::LengthMeters - point.x;
    }

    double xFromSignedProgress(double signedProgress, AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? signedProgress
            : PitchGeometry::LengthMeters - signedProgress;
    }

    double ballSideOffset(const TeamShapeContext& context, double maxMeters) {
        const double centerY = PitchGeometry::WidthMeters / 2.0;
        const double normalizedSide = std::clamp(
            (context.ballPosition.y - centerY) / centerY,
            -1.0,
            1.0);
        return normalizedSide * maxMeters;
    }

    double sideSignForRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::LeftWinger:
            return -1.0;
        case FormationSlotRole::RightBack:
        case FormationSlotRole::RightWingBack:
        case FormationSlotRole::RightMidfielder:
        case FormationSlotRole::RightWinger:
            return 1.0;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::Striker:
            return 0.0;
        }

        return 0.0;
    }

    double centralChannelShare(PitchPoint point) {
        const double distanceFromCenter =
            std::abs(point.y - PitchGeometry::WidthMeters / 2.0);
        return std::clamp(1.0 - (distanceFromCenter / (PitchGeometry::WidthMeters * 0.24)), 0.0, 1.0);
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

    bool isRestDefenseRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::DefensiveMidfielder:
            return true;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
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

    bool isAdvancedSupportRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::AttackingMidfielder:
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
        case FormationSlotRole::Striker:
            return true;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
        case FormationSlotRole::DefensiveMidfielder:
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
            return false;
        }

        return false;
    }

    double minimumShapeGoalDistance(FormationSlotRole role, PitchPoint target) {
        if (isRestDefenseRole(role)) {
            return kDynamicShapeProfile.restDefenseMinimumGoalDistance;
        }

        const double centralShare = centralChannelShare(target);
        if (isAdvancedSupportRole(role)) {
            return kDynamicShapeProfile.advancedCentralMinimumGoalDistance * centralShare
                + kDynamicShapeProfile.advancedWideMinimumGoalDistance * (1.0 - centralShare);
        }

        return kDynamicShapeProfile.supportCentralMinimumGoalDistance * centralShare
            + kDynamicShapeProfile.supportWideMinimumGoalDistance * (1.0 - centralShare);
    }

    PitchPoint enforceOpenPlayShapeDepth(
        PitchPoint target,
        FormationSlotRole role,
        AttackingDirection direction) {
        if (role == FormationSlotRole::Goalkeeper) {
            return target;
        }

        const double minimumDistance = minimumShapeGoalDistance(role, target);
        const double maxProgress = PitchGeometry::LengthMeters - minimumDistance;
        const double progress = signedProgressX(target, direction);
        if (progress <= maxProgress) {
            return target;
        }

        return PitchPoint{
            xFromSignedProgress(std::clamp(maxProgress, 0.0, PitchGeometry::LengthMeters), direction),
            target.y
        };
    }

    MentalityShapeProfile mentalityProfile(TeamMentality mentality) {
        switch (mentality) {
        case TeamMentality::Defensive:
            return MentalityShapeProfile{ 2.5, 0.75, 10.0 };
        case TeamMentality::Balanced:
            return MentalityShapeProfile{ 1.75, 1.0, 7.5 };
        case TeamMentality::Attacking:
            return MentalityShapeProfile{ 1.0, 1.18, 5.5 };
        }

        return MentalityShapeProfile{ 1.75, 1.0, 7.5 };
    }

    double roleLineWeight(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            return 0.0;
        case FormationSlotRole::CenterBack:
            return 0.15;
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
            return 0.28;
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            return 0.42;
        case FormationSlotRole::DefensiveMidfielder:
            return 0.34;
        case FormationSlotRole::CentralMidfielder:
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
            return 0.58;
        case FormationSlotRole::AttackingMidfielder:
            return 0.76;
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            return 0.82;
        case FormationSlotRole::Striker:
            return 0.92;
        case FormationSlotRole::Unknown:
            return 0.50;
        }

        return 0.50;
    }

    RoleShapeProfile roleShapeProfile(FormationSlotRole role) {
        const double lineWeight = roleLineWeight(role);
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            return RoleShapeProfile{ lineWeight, 0.12, 1.0, 0.05, 12.0 };
        case FormationSlotRole::CenterBack:
            return RoleShapeProfile{ lineWeight, 0.34, 0.85, 0.12, 5.0 };
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
            return RoleShapeProfile{ lineWeight, 0.58, 0.72, 0.36, 0.0 };
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            return RoleShapeProfile{ lineWeight, 0.70, 0.62, 0.44, 0.0 };
        case FormationSlotRole::DefensiveMidfielder:
            return RoleShapeProfile{ lineWeight, 0.45, 0.78, 0.32, 2.5 };
        case FormationSlotRole::CentralMidfielder:
            return RoleShapeProfile{ lineWeight, 0.68, 0.55, 0.56, 0.0 };
        case FormationSlotRole::AttackingMidfielder:
            return RoleShapeProfile{ lineWeight, 0.72, 0.42, 0.58, 0.0 };
        case FormationSlotRole::LeftMidfielder:
        case FormationSlotRole::RightMidfielder:
            return RoleShapeProfile{ lineWeight, 0.78, 0.70, 0.50, 0.0 };
        case FormationSlotRole::LeftWinger:
        case FormationSlotRole::RightWinger:
            return RoleShapeProfile{ lineWeight, 0.82, 0.82, 0.52, 0.0 };
        case FormationSlotRole::Striker:
            return RoleShapeProfile{ lineWeight, 0.58, 0.30, 0.42, 0.0 };
        case FormationSlotRole::Unknown:
            return RoleShapeProfile{ lineWeight, 0.45, 0.45, 0.35, 0.0 };
        }

        return RoleShapeProfile{ lineWeight, 0.45, 0.45, 0.35, 0.0 };
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

    PitchPoint blend(PitchPoint from, PitchPoint to, double weight) {
        const double clampedWeight = std::clamp(weight, 0.0, 1.0);
        return PitchPoint{
            from.x + (to.x - from.x) * clampedWeight,
            from.y + (to.y - from.y) * clampedWeight
        };
    }

    double proximityWeight(PitchPoint a, PitchPoint b, double radiusMeters) {
        if (radiusMeters <= 0.0) {
            return 0.0;
        }

        return std::clamp(1.0 - (PitchGeometry::distance(a, b) / radiusMeters), 0.0, 1.0);
    }

    PitchPoint enforceRestDefenseSpacing(
        PitchPoint target,
        FormationSlotRole role,
        const TeamShapeContext& context,
        double gapMeters) {
        if (!isRestDefenseRole(role)) {
            return target;
        }

        const double maxProgress = signedProgressX(context.ballPosition, context.attackingDirection)
            - std::max(0.0, gapMeters);
        const double targetProgress = signedProgressX(target, context.attackingDirection);
        if (targetProgress <= maxProgress) {
            return target;
        }

        return PitchPoint{
            xFromSignedProgress(std::clamp(maxProgress, 0.0, PitchGeometry::LengthMeters), context.attackingDirection),
            target.y
        };
    }

    PitchPoint applyInPossessionShapeAdjustment(
        PitchPoint tacticalPosition,
        FormationSlotRole role,
        const TeamShapeContext& context,
        const RoleShapeProfile& roleProfile,
        const MentalityShapeProfile& mentality) {
        PitchPoint adjusted = tacticalPosition;
        const double ballShift = ballSideOffset(context, kDynamicShapeProfile.possessionBallShiftMeters)
            * roleProfile.ballSideShiftWeight;

        const double roleSide = sideSignForRole(role);
        const double ballSide = ballSideOffset(context, 1.0);
        const bool ballSideWideRole = isWideRole(role)
            && roleSide != 0.0
            && ballSide != 0.0
            && (roleSide > 0.0) == (ballSide > 0.0);
        const bool farSideAdvancedRole = isAdvancedSupportRole(role)
            && roleSide != 0.0
            && ballSide != 0.0
            && (roleSide > 0.0) != (ballSide > 0.0);

        const double widthHold = ballSideWideRole ? roleProfile.widthHoldWeight * 0.45 : roleProfile.widthHoldWeight;
        adjusted.y += ballShift * (1.0 - widthHold);

        if (ballSideWideRole) {
            adjusted = blend(adjusted, PitchPoint{ adjusted.x, context.ballPosition.y }, 0.02);
        } else if (farSideAdvancedRole) {
            const double centerY = PitchGeometry::WidthMeters / 2.0;
            adjusted.y += (centerY - adjusted.y)
                * std::clamp(kDynamicShapeProfile.possessionFarSideTuckMeters / PitchGeometry::WidthMeters, 0.0, 1.0);
        }

        adjusted = enforceRestDefenseSpacing(
            adjusted,
            role,
            context,
            mentality.restDefenseGapMeters + roleProfile.restDefenseGapBonusMeters);
        return adjusted;
    }

    PitchPoint applyOutOfPossessionShapeAdjustment(
        PitchPoint tacticalPosition,
        FormationSlotRole role,
        const TeamShapeContext& context,
        const RoleShapeProfile& roleProfile) {
        PitchPoint adjusted = tacticalPosition;
        const double ballShift = ballSideOffset(context, kDynamicShapeProfile.defensiveBallShiftMeters)
            * roleProfile.ballSideShiftWeight
            * (1.0 - roleProfile.widthHoldWeight * 0.45);
        adjusted.y += ballShift;

        const double centerY = PitchGeometry::WidthMeters / 2.0;
        adjusted.y += (centerY - adjusted.y)
            * (kDynamicShapeProfile.defensiveCompactTuckMeters / PitchGeometry::WidthMeters)
            * (1.0 - roleProfile.widthHoldWeight * 0.35);

        if (role == FormationSlotRole::Goalkeeper) {
            adjusted.y += ballSideOffset(context, kDynamicShapeProfile.goalkeeperDefensiveShiftMeters);
        }

        return adjusted;
    }

    PitchPoint applyAttackingTransitionShapeAdjustment(
        PitchPoint tacticalPosition,
        FormationSlotRole role,
        const TeamShapeContext& context,
        const RoleShapeProfile& roleProfile,
        const MentalityShapeProfile& mentality) {
        PitchPoint adjusted = applyInPossessionShapeAdjustment(
            tacticalPosition,
            role,
            context,
            roleProfile,
            mentality);

        adjusted = enforceRestDefenseSpacing(
            adjusted,
            role,
            context,
            mentality.restDefenseGapMeters + roleProfile.restDefenseGapBonusMeters);
        return adjusted;
    }

    PitchPoint applyDefensiveTransitionShapeAdjustment(
        PitchPoint tacticalPosition,
        FormationSlotRole role,
        const TeamShapeContext& context,
        const RoleShapeProfile& roleProfile,
        const MentalityShapeProfile& mentality) {
        PitchPoint adjusted = applyOutOfPossessionShapeAdjustment(
            tacticalPosition,
            role,
            context,
            roleProfile);
        const double nearby = proximityWeight(
            tacticalPosition,
            context.ballPosition,
            kDynamicShapeProfile.counterPressProximityMeters);
        const double pressBlend = kDynamicShapeProfile.transitionCounterPressBlend
            * roleProfile.counterPressWeight
            * mentality.counterPressMultiplier
            * nearby;

        adjusted = blend(adjusted, context.ballPosition, pressBlend);
        adjusted.x -= attackSign(context.attackingDirection)
            * mentality.defensiveRecoveryMeters
            * (1.0 - nearby)
            * (1.0 - roleProfile.lineWeight * 0.45);
        return adjusted;
    }

    PitchPoint applyPhaseShapeAdjustment(
        PitchPoint tacticalPosition,
        FormationSlotRole role,
        const TeamShapeContext& context) {
        const RoleShapeProfile roleProfile = roleShapeProfile(role);
        const MentalityShapeProfile mentality = mentalityProfile(context.tacticalSetup.mentality);

        PitchPoint adjusted = tacticalPosition;
        switch (context.phase) {
        case TeamShapePhase::InPossession:
            adjusted = applyInPossessionShapeAdjustment(
                tacticalPosition,
                role,
                context,
                roleProfile,
                mentality);
            break;
        case TeamShapePhase::OutOfPossession:
            adjusted = applyOutOfPossessionShapeAdjustment(
                tacticalPosition,
                role,
                context,
                roleProfile);
            break;
        case TeamShapePhase::AttackingTransition:
            adjusted = applyAttackingTransitionShapeAdjustment(
                tacticalPosition,
                role,
                context,
                roleProfile,
                mentality);
            break;
        case TeamShapePhase::DefensiveTransition:
            adjusted = applyDefensiveTransitionShapeAdjustment(
                tacticalPosition,
                role,
                context,
                roleProfile,
                mentality);
            break;
        }

        return PitchGeometry::clampToPitch(
            enforceOpenPlayShapeDepth(adjusted, role, context.attackingDirection));
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
            applyPhaseShapeAdjustment(tacticalPosition, slotRole, context)
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
