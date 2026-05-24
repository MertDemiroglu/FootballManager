#include"fm/match_engine/TacticalZone.h"

#include<algorithm>
#include<cmath>

namespace {
    enum class ZoneLane {
        Unknown,
        Left,
        Center,
        Right
    };

    enum class ZoneThird {
        Unknown,
        Defensive,
        Middle,
        Attacking
    };

    double attackingProgress(PitchPoint point, AttackingDirection direction) {
        const PitchPoint clamped = PitchGeometry::clampToPitch(point);
        if (direction == AttackingDirection::HomeToAway) {
            return clamped.x / PitchGeometry::LengthMeters;
        }

        return (PitchGeometry::LengthMeters - clamped.x) / PitchGeometry::LengthMeters;
    }

    double attackingLateral(PitchPoint point, AttackingDirection direction) {
        const PitchPoint clamped = PitchGeometry::clampToPitch(point);
        if (direction == AttackingDirection::HomeToAway) {
            return clamped.y / PitchGeometry::WidthMeters;
        }

        return (PitchGeometry::WidthMeters - clamped.y) / PitchGeometry::WidthMeters;
    }

    ZoneLane laneFor(TacticalZone zone) {
        switch (zone) {
        case TacticalZone::DefensiveLeft:
        case TacticalZone::MiddleLeft:
        case TacticalZone::AttackingLeft:
            return ZoneLane::Left;
        case TacticalZone::DefensiveCenter:
        case TacticalZone::MiddleCenter:
        case TacticalZone::AttackingCenter:
            return ZoneLane::Center;
        case TacticalZone::DefensiveRight:
        case TacticalZone::MiddleRight:
        case TacticalZone::AttackingRight:
            return ZoneLane::Right;
        case TacticalZone::Unknown:
            return ZoneLane::Unknown;
        }

        return ZoneLane::Unknown;
    }

    ZoneThird thirdFor(TacticalZone zone) {
        switch (zone) {
        case TacticalZone::DefensiveLeft:
        case TacticalZone::DefensiveCenter:
        case TacticalZone::DefensiveRight:
            return ZoneThird::Defensive;
        case TacticalZone::MiddleLeft:
        case TacticalZone::MiddleCenter:
        case TacticalZone::MiddleRight:
            return ZoneThird::Middle;
        case TacticalZone::AttackingLeft:
        case TacticalZone::AttackingCenter:
        case TacticalZone::AttackingRight:
            return ZoneThird::Attacking;
        case TacticalZone::Unknown:
            return ZoneThird::Unknown;
        }

        return ZoneThird::Unknown;
    }

    int laneIndex(TacticalZone zone) {
        switch (laneFor(zone)) {
        case ZoneLane::Left:
            return 0;
        case ZoneLane::Center:
            return 1;
        case ZoneLane::Right:
            return 2;
        case ZoneLane::Unknown:
            return -1;
        }

        return -1;
    }

    TacticalZone zoneFor(ZoneThird third, ZoneLane lane) {
        if (third == ZoneThird::Defensive) {
            if (lane == ZoneLane::Left) return TacticalZone::DefensiveLeft;
            if (lane == ZoneLane::Center) return TacticalZone::DefensiveCenter;
            if (lane == ZoneLane::Right) return TacticalZone::DefensiveRight;
        }
        if (third == ZoneThird::Middle) {
            if (lane == ZoneLane::Left) return TacticalZone::MiddleLeft;
            if (lane == ZoneLane::Center) return TacticalZone::MiddleCenter;
            if (lane == ZoneLane::Right) return TacticalZone::MiddleRight;
        }
        if (third == ZoneThird::Attacking) {
            if (lane == ZoneLane::Left) return TacticalZone::AttackingLeft;
            if (lane == ZoneLane::Center) return TacticalZone::AttackingCenter;
            if (lane == ZoneLane::Right) return TacticalZone::AttackingRight;
        }

        return TacticalZone::Unknown;
    }
}

TacticalZone tacticalZoneForPoint(PitchPoint point, AttackingDirection direction) {
    const double progress = std::clamp(attackingProgress(point, direction), 0.0, 1.0);
    const double lateral = std::clamp(attackingLateral(point, direction), 0.0, 1.0);

    ZoneThird third = ZoneThird::Attacking;
    if (progress < (1.0 / 3.0)) {
        third = ZoneThird::Defensive;
    } else if (progress < (2.0 / 3.0)) {
        third = ZoneThird::Middle;
    }

    ZoneLane lane = ZoneLane::Right;
    if (lateral < (1.0 / 3.0)) {
        lane = ZoneLane::Left;
    } else if (lateral < (2.0 / 3.0)) {
        lane = ZoneLane::Center;
    }

    return zoneFor(third, lane);
}

bool isWideZone(TacticalZone zone) {
    const ZoneLane lane = laneFor(zone);
    return lane == ZoneLane::Left || lane == ZoneLane::Right;
}

bool isCentralZone(TacticalZone zone) {
    return laneFor(zone) == ZoneLane::Center;
}

bool isDefensiveThird(TacticalZone zone) {
    return thirdFor(zone) == ZoneThird::Defensive;
}

bool isMiddleThird(TacticalZone zone) {
    return thirdFor(zone) == ZoneThird::Middle;
}

bool isAttackingThird(TacticalZone zone) {
    return thirdFor(zone) == ZoneThird::Attacking;
}

bool sameVerticalLane(TacticalZone a, TacticalZone b) {
    const int lhs = laneIndex(a);
    const int rhs = laneIndex(b);
    return lhs >= 0 && lhs == rhs;
}

bool adjacentLane(TacticalZone a, TacticalZone b) {
    const int lhs = laneIndex(a);
    const int rhs = laneIndex(b);
    return lhs >= 0 && rhs >= 0 && std::abs(lhs - rhs) == 1;
}

bool sameOrAdjacentLane(TacticalZone a, TacticalZone b) {
    return sameVerticalLane(a, b) || adjacentLane(a, b);
}
