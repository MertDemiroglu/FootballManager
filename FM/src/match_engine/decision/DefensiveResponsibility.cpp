#include"fm/match_engine/decision/DefensiveResponsibility.h"

#include<algorithm>

namespace {
    bool containsRole(
        const std::vector<FormationSlotRole>& roles,
        FormationSlotRole role) {
        return std::find(roles.begin(), roles.end(), role) != roles.end();
    }

    bool containsZone(
        const std::vector<TacticalZone>& zones,
        TacticalZone zone) {
        return std::find(zones.begin(), zones.end(), zone) != zones.end();
    }

    bool isDefensiveWideRole(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::LeftBack:
        case FormationSlotRole::RightBack:
        case FormationSlotRole::LeftWingBack:
        case FormationSlotRole::RightWingBack:
            return true;
        case FormationSlotRole::Unknown:
        case FormationSlotRole::Goalkeeper:
        case FormationSlotRole::CenterBack:
        case FormationSlotRole::DefensiveMidfielder:
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

    bool roleHasLaneAccess(
        const DefensiveResponsibility& responsibility,
        TacticalZone zone) {
        if (responsibility.primaryZone != TacticalZone::Unknown
            && sameOrAdjacentLane(responsibility.primaryZone, zone)) {
            return true;
        }

        for (TacticalZone secondaryZone : responsibility.secondaryZones) {
            if (secondaryZone != TacticalZone::Unknown
                && sameOrAdjacentLane(secondaryZone, zone)) {
                return true;
            }
        }

        return false;
    }

    bool veryCloseDefensiveEmergency(
        const DefensiveResponsibility& responsibility,
        TacticalZone ballZone,
        FormationSlotRole opponentRole,
        double distanceToBallCarrier) {
        return distanceToBallCarrier <= 4.0
            && isDefensiveThird(ballZone)
            && isDefensiveWideRole(responsibility.role)
            && opponentRole == FormationSlotRole::Striker;
    }

    std::vector<FormationSlotRole> widePressableRoles() {
        return {
            FormationSlotRole::LeftBack,
            FormationSlotRole::RightBack,
            FormationSlotRole::LeftWingBack,
            FormationSlotRole::RightWingBack,
            FormationSlotRole::LeftMidfielder,
            FormationSlotRole::RightMidfielder,
            FormationSlotRole::LeftWinger,
            FormationSlotRole::RightWinger
        };
    }
}

DefensiveResponsibility buildDefensiveResponsibility(
    PlayerId playerId,
    FormationSlotRole role,
    AttackingDirection direction) {
    (void)direction;

    DefensiveResponsibility responsibility;
    responsibility.playerId = playerId;
    responsibility.role = role;

    switch (role) {
    case FormationSlotRole::Goalkeeper:
        responsibility.primaryZone = TacticalZone::DefensiveCenter;
        break;

    case FormationSlotRole::CenterBack:
        responsibility.primaryZone = TacticalZone::DefensiveCenter;
        responsibility.secondaryZones = {
            TacticalZone::DefensiveLeft,
            TacticalZone::DefensiveRight
        };
        responsibility.pressableOpponentRoles = {
            FormationSlotRole::Striker,
            FormationSlotRole::AttackingMidfielder,
            FormationSlotRole::CentralMidfielder
        };
        break;

    case FormationSlotRole::LeftBack:
        responsibility.primaryZone = TacticalZone::DefensiveLeft;
        responsibility.secondaryZones = { TacticalZone::MiddleLeft };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::RightBack:
        responsibility.primaryZone = TacticalZone::DefensiveRight;
        responsibility.secondaryZones = { TacticalZone::MiddleRight };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::LeftWingBack:
        responsibility.primaryZone = TacticalZone::DefensiveLeft;
        responsibility.secondaryZones = {
            TacticalZone::MiddleLeft,
            TacticalZone::AttackingLeft
        };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::RightWingBack:
        responsibility.primaryZone = TacticalZone::DefensiveRight;
        responsibility.secondaryZones = {
            TacticalZone::MiddleRight,
            TacticalZone::AttackingRight
        };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::DefensiveMidfielder:
        responsibility.primaryZone = TacticalZone::MiddleCenter;
        responsibility.secondaryZones = {
            TacticalZone::DefensiveCenter,
            TacticalZone::DefensiveLeft,
            TacticalZone::DefensiveRight,
            TacticalZone::MiddleLeft,
            TacticalZone::MiddleRight
        };
        responsibility.pressableOpponentRoles = {
            FormationSlotRole::AttackingMidfielder,
            FormationSlotRole::CentralMidfielder,
            FormationSlotRole::DefensiveMidfielder,
            FormationSlotRole::Striker
        };
        break;

    case FormationSlotRole::CentralMidfielder:
        responsibility.primaryZone = TacticalZone::MiddleCenter;
        responsibility.secondaryZones = {
            TacticalZone::MiddleLeft,
            TacticalZone::MiddleRight
        };
        responsibility.pressableOpponentRoles = {
            FormationSlotRole::CentralMidfielder,
            FormationSlotRole::DefensiveMidfielder,
            FormationSlotRole::AttackingMidfielder
        };
        break;

    case FormationSlotRole::AttackingMidfielder:
        responsibility.primaryZone = TacticalZone::AttackingCenter;
        responsibility.secondaryZones = { TacticalZone::MiddleCenter };
        responsibility.pressableOpponentRoles = {
            FormationSlotRole::DefensiveMidfielder,
            FormationSlotRole::CentralMidfielder,
            FormationSlotRole::CenterBack
        };
        break;

    case FormationSlotRole::LeftMidfielder:
        responsibility.primaryZone = TacticalZone::MiddleLeft;
        responsibility.secondaryZones = {
            TacticalZone::DefensiveLeft,
            TacticalZone::AttackingLeft
        };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::RightMidfielder:
        responsibility.primaryZone = TacticalZone::MiddleRight;
        responsibility.secondaryZones = {
            TacticalZone::DefensiveRight,
            TacticalZone::AttackingRight
        };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::LeftWinger:
        responsibility.primaryZone = TacticalZone::AttackingLeft;
        responsibility.secondaryZones = { TacticalZone::MiddleLeft };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::RightWinger:
        responsibility.primaryZone = TacticalZone::AttackingRight;
        responsibility.secondaryZones = { TacticalZone::MiddleRight };
        responsibility.pressableOpponentRoles = widePressableRoles();
        break;

    case FormationSlotRole::Striker:
        responsibility.primaryZone = TacticalZone::AttackingCenter;
        responsibility.secondaryZones = { TacticalZone::MiddleCenter };
        responsibility.pressableOpponentRoles = {
            FormationSlotRole::CenterBack,
            FormationSlotRole::DefensiveMidfielder
        };
        break;

    case FormationSlotRole::Unknown:
        responsibility.primaryZone = TacticalZone::Unknown;
        break;
    }

    return responsibility;
}

bool isZoneInResponsibility(
    const DefensiveResponsibility& responsibility,
    TacticalZone zone) {
    if (zone == TacticalZone::Unknown) {
        return false;
    }

    return responsibility.primaryZone == zone
        || containsZone(responsibility.secondaryZones, zone);
}

bool isOpponentRolePressable(
    const DefensiveResponsibility& responsibility,
    FormationSlotRole opponentRole) {
    if (opponentRole == FormationSlotRole::Unknown) {
        return true;
    }

    return containsRole(responsibility.pressableOpponentRoles, opponentRole);
}

bool canConsiderPressing(
    const DefensiveResponsibility& responsibility,
    TacticalZone ballZone,
    FormationSlotRole opponentBallCarrierRole,
    double distanceToBallCarrier,
    MarkingStyle markingStyle) {
    if (responsibility.role == FormationSlotRole::Goalkeeper
        || ballZone == TacticalZone::Unknown
        || distanceToBallCarrier < 0.0) {
        return false;
    }

    double maxPressDistance = 8.0;
    if (markingStyle == MarkingStyle::Zonal) {
        maxPressDistance = 7.0;
    } else if (markingStyle == MarkingStyle::ManOriented) {
        maxPressDistance = 9.0;
    }

    if (distanceToBallCarrier > maxPressDistance) {
        return false;
    }

    const bool responsibleZone = isZoneInResponsibility(responsibility, ballZone);
    const bool laneRelevant = roleHasLaneAccess(responsibility, ballZone);
    if (!responsibleZone && !laneRelevant) {
        return false;
    }

    if (isOpponentRolePressable(responsibility, opponentBallCarrierRole)) {
        return true;
    }

    return veryCloseDefensiveEmergency(
        responsibility,
        ballZone,
        opponentBallCarrierRole,
        distanceToBallCarrier);
}
