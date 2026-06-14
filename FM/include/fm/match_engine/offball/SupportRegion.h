#pragma once

#include"fm/common/Types.h"
#include"fm/match_engine/geometry/PitchGeometry.h"
#include"fm/match_engine/movement/TeamShapeModel.h"
#include"fm/match_engine/offball/OffBallEventTypes.h"
#include"fm/roster/FormationSlotRole.h"

#include<algorithm>

struct SupportRegion {
    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;
    SupportRegionLane preferredLane = SupportRegionLane::Any;
    SupportRegionDepth preferredDepth = SupportRegionDepth::Any;
    FormationSlotRole preferredRole = FormationSlotRole::Unknown;
    double roleSuitability = 1.0;
    AttackingDirection attackingDirection = AttackingDirection::HomeToAway;

    PitchPoint center() const {
        return PitchGeometry::clampToPitch(PitchPoint{
            (minX + maxX) / 2.0,
            (minY + maxY) / 2.0
        });
    }

    bool contains(PitchPoint point) const {
        const PitchPoint clamped = PitchGeometry::clampToPitch(point);
        return clamped.x >= std::min(minX, maxX)
            && clamped.x <= std::max(minX, maxX)
            && clamped.y >= std::min(minY, maxY)
            && clamped.y <= std::max(minY, maxY);
    }
};

inline SupportRegion clampSupportRegion(SupportRegion region) {
    region.minX = std::clamp(region.minX, 0.0, PitchGeometry::LengthMeters);
    region.maxX = std::clamp(region.maxX, 0.0, PitchGeometry::LengthMeters);
    region.minY = std::clamp(region.minY, 0.0, PitchGeometry::WidthMeters);
    region.maxY = std::clamp(region.maxY, 0.0, PitchGeometry::WidthMeters);
    if (region.minX > region.maxX) {
        std::swap(region.minX, region.maxX);
    }
    if (region.minY > region.maxY) {
        std::swap(region.minY, region.maxY);
    }
    return region;
}
