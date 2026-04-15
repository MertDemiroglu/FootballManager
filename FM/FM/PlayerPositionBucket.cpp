#include "PlayerPositionBucket.h"

FormationSlotGroup mapPlayerPositionToFormationSlotGroup(PlayerPosition position) {
    switch (position) {
    case PlayerPosition::Goalkeeper:
        return FormationSlotGroup::Goalkeeper;

    case PlayerPosition::CenterBack:
    case PlayerPosition::LeftBack:
    case PlayerPosition::RightBack:
        return FormationSlotGroup::Defender;

    case PlayerPosition::DefensiveMidfielder:
    case PlayerPosition::CentralMidfielder:
    case PlayerPosition::AttackingMidfielder:
    case PlayerPosition::LeftMidfielder:
    case PlayerPosition::RightMidfielder:
    case PlayerPosition::LeftWinger:
    case PlayerPosition::RightWinger:
        return FormationSlotGroup::Midfielder;

    case PlayerPosition::Striker:
        return FormationSlotGroup::Forward;

    case PlayerPosition::Unknown:
        break;
    }

    return FormationSlotGroup::Unknown;
}

FormationSlotGroup mapPlayerPositionToBucket(PlayerPosition position) {
    return mapPlayerPositionToFormationSlotGroup(position);
}
