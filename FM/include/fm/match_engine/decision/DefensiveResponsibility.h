#pragma once

#include"fm/common/Types.h"
#include"fm/match/TacticalSetup.h"
#include"fm/match_engine/geometry/TacticalZones.h"
#include"fm/roster/FormationSlotRole.h"

#include<vector>

struct DefensiveResponsibility {
    PlayerId playerId = 0;
    FormationSlotRole role = FormationSlotRole::Unknown;

    TacticalZone primaryZone = TacticalZone::Unknown;
    std::vector<TacticalZone> secondaryZones;

    std::vector<FormationSlotRole> pressableOpponentRoles;
};

DefensiveResponsibility buildDefensiveResponsibility(
    PlayerId playerId,
    FormationSlotRole role,
    AttackingDirection direction);

bool isZoneInResponsibility(
    const DefensiveResponsibility& responsibility,
    TacticalZone zone);

bool isOpponentRolePressable(
    const DefensiveResponsibility& responsibility,
    FormationSlotRole opponentRole);

bool canConsiderPressing(
    const DefensiveResponsibility& responsibility,
    TacticalZone ballZone,
    FormationSlotRole opponentBallCarrierRole,
    double distanceToBallCarrier,
    MarkingStyle markingStyle);
