#include"fm/roster/PlayerRoleFit.h"

#include"fm/roster/Footballer.h"

namespace {
RoleFitTier fitForLeftMidfielder(PlayerPosition playerPosition) {
    if (playerPosition == PlayerPosition::LeftMidfielder) return RoleFitTier::Natural;
    if (playerPosition == PlayerPosition::CentralMidfielder || playerPosition == PlayerPosition::LeftWinger) return RoleFitTier::Close;
    if (playerPosition == PlayerPosition::LeftBack || playerPosition == PlayerPosition::AttackingMidfielder || playerPosition == PlayerPosition::DefensiveMidfielder) return RoleFitTier::Adapted;
    if (playerPosition == PlayerPosition::RightMidfielder || playerPosition == PlayerPosition::RightWinger) return RoleFitTier::Emergency;
    if (playerPosition == PlayerPosition::Striker || playerPosition == PlayerPosition::CenterBack || playerPosition == PlayerPosition::RightBack) return RoleFitTier::SevereMismatch;
    return RoleFitTier::Invalid;
}

RoleFitTier fitForRightMidfielder(PlayerPosition playerPosition) {
    if (playerPosition == PlayerPosition::RightMidfielder) return RoleFitTier::Natural;
    if (playerPosition == PlayerPosition::CentralMidfielder || playerPosition == PlayerPosition::RightWinger) return RoleFitTier::Close;
    if (playerPosition == PlayerPosition::RightBack || playerPosition == PlayerPosition::AttackingMidfielder || playerPosition == PlayerPosition::DefensiveMidfielder) return RoleFitTier::Adapted;
    if (playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::LeftWinger) return RoleFitTier::Emergency;
    if (playerPosition == PlayerPosition::Striker || playerPosition == PlayerPosition::CenterBack || playerPosition == PlayerPosition::LeftBack) return RoleFitTier::SevereMismatch;
    return RoleFitTier::Invalid;
}
}

RoleFitTier getRoleFitTier(PlayerPosition playerPosition, FormationSlotRole slotRole) {
    if (slotRole == FormationSlotRole::Unknown || playerPosition == PlayerPosition::Unknown) {
        return RoleFitTier::Invalid;
    }

    if (slotRole == FormationSlotRole::Goalkeeper) {
        return playerPosition == PlayerPosition::Goalkeeper ? RoleFitTier::Natural : RoleFitTier::Invalid;
    }

    if (playerPosition == PlayerPosition::Goalkeeper) {
        return RoleFitTier::Invalid;
    }

    switch (slotRole) {
    case FormationSlotRole::CenterBack:
        if (playerPosition == PlayerPosition::CenterBack) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::DefensiveMidfielder) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::LeftBack || playerPosition == PlayerPosition::RightBack) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Emergency;
        if (playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::SevereMismatch;
        return RoleFitTier::Invalid;

    case FormationSlotRole::LeftBack:
        if (playerPosition == PlayerPosition::LeftBack) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::CenterBack) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::LeftWinger || playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::DefensiveMidfielder) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::RightBack || playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Emergency;
        if (playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::SevereMismatch;
        return RoleFitTier::Invalid;

    case FormationSlotRole::RightBack:
        if (playerPosition == PlayerPosition::RightBack) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::CenterBack) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::RightWinger || playerPosition == PlayerPosition::RightMidfielder || playerPosition == PlayerPosition::DefensiveMidfielder) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::LeftBack || playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Emergency;
        if (playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::SevereMismatch;
        return RoleFitTier::Invalid;

    case FormationSlotRole::LeftWingBack:
        if (playerPosition == PlayerPosition::LeftBack || playerPosition == PlayerPosition::LeftWinger) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::CenterBack || playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::DefensiveMidfielder || playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::Emergency;
        return RoleFitTier::Invalid;

    case FormationSlotRole::RightWingBack:
        if (playerPosition == PlayerPosition::RightBack || playerPosition == PlayerPosition::RightWinger) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::RightMidfielder || playerPosition == PlayerPosition::CenterBack || playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::DefensiveMidfielder || playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::Emergency;
        return RoleFitTier::Invalid;

    case FormationSlotRole::DefensiveMidfielder:
        if (playerPosition == PlayerPosition::DefensiveMidfielder) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::CenterBack || playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::RightMidfielder) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::Emergency;
        return RoleFitTier::Invalid;

    case FormationSlotRole::CentralMidfielder:
        if (playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::DefensiveMidfielder || playerPosition == PlayerPosition::AttackingMidfielder || playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::RightMidfielder) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::LeftWinger || playerPosition == PlayerPosition::RightWinger) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::CenterBack || playerPosition == PlayerPosition::Striker) return RoleFitTier::Emergency;
        return RoleFitTier::Invalid;

    case FormationSlotRole::AttackingMidfielder:
        if (playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::CentralMidfielder || playerPosition == PlayerPosition::LeftWinger || playerPosition == PlayerPosition::RightWinger || playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::RightMidfielder) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::Striker) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::DefensiveMidfielder) return RoleFitTier::Emergency;
        if (playerPosition == PlayerPosition::CenterBack) return RoleFitTier::SevereMismatch;
        return RoleFitTier::Invalid;

    case FormationSlotRole::LeftMidfielder:
        return fitForLeftMidfielder(playerPosition);

    case FormationSlotRole::RightMidfielder:
        return fitForRightMidfielder(playerPosition);

    case FormationSlotRole::LeftWinger:
        if (playerPosition == PlayerPosition::LeftWinger) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::AttackingMidfielder || playerPosition == PlayerPosition::LeftMidfielder) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::RightWinger || playerPosition == PlayerPosition::Striker || playerPosition == PlayerPosition::LeftBack) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::RightMidfielder || playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Emergency;
        return RoleFitTier::Invalid;

    case FormationSlotRole::RightWinger:
        if (playerPosition == PlayerPosition::RightWinger) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::AttackingMidfielder || playerPosition == PlayerPosition::RightMidfielder) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::LeftWinger || playerPosition == PlayerPosition::Striker || playerPosition == PlayerPosition::RightBack) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::Emergency;
        return RoleFitTier::Invalid;

    case FormationSlotRole::Striker:
        if (playerPosition == PlayerPosition::Striker) return RoleFitTier::Natural;
        if (playerPosition == PlayerPosition::AttackingMidfielder) return RoleFitTier::Close;
        if (playerPosition == PlayerPosition::LeftWinger || playerPosition == PlayerPosition::RightWinger || playerPosition == PlayerPosition::LeftMidfielder || playerPosition == PlayerPosition::RightMidfielder) return RoleFitTier::Adapted;
        if (playerPosition == PlayerPosition::CentralMidfielder) return RoleFitTier::SevereMismatch;
        return RoleFitTier::Invalid;

    case FormationSlotRole::Goalkeeper:
    case FormationSlotRole::Unknown:
        break;
    }

    return RoleFitTier::Invalid;
}

double getRoleFitMultiplier(RoleFitTier tier) {
    switch (tier) {
    case RoleFitTier::Natural:
        return 1.00;
    case RoleFitTier::Close:
        return 0.92;
    case RoleFitTier::Adapted:
        return 0.82;
    case RoleFitTier::Emergency:
        return 0.65;
    case RoleFitTier::SevereMismatch:
        return 0.40;
    case RoleFitTier::Invalid:
        return 0.00;
    }

    return 0.00;
}

double getRoleFitMultiplier(PlayerPosition playerPosition, FormationSlotRole slotRole) {
    return getRoleFitMultiplier(getRoleFitTier(playerPosition, slotRole));
}

bool isRoleFitUsable(PlayerPosition playerPosition, FormationSlotRole slotRole) {
    return getRoleFitTier(playerPosition, slotRole) != RoleFitTier::Invalid;
}

int getRoleFitTierScore(RoleFitTier tier) {
    switch (tier) {
    case RoleFitTier::Natural:
        return 5;
    case RoleFitTier::Close:
        return 4;
    case RoleFitTier::Adapted:
        return 3;
    case RoleFitTier::Emergency:
        return 2;
    case RoleFitTier::SevereMismatch:
        return 1;
    case RoleFitTier::Invalid:
        return 0;
    }

    return 0;
}

double calculateEffectivePowerForSlot(const Footballer& player, FormationSlotRole slotRole) {
    const double multiplier = getRoleFitMultiplier(player.getPlayerPosition(), slotRole);
    return static_cast<double>(player.totalPower()) * multiplier;
}
