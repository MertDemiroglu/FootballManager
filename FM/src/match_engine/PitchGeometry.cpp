#include"fm/match_engine/PitchGeometry.h"

#include<algorithm>
#include<cmath>

namespace {
    constexpr double centerY() {
        return PitchGeometry::WidthMeters / 2.0;
    }

    bool isInsideCenteredArea(PitchPoint point, double minX, double maxX, double areaWidth) {
        const double halfWidth = areaWidth / 2.0;
        const double minY = centerY() - halfWidth;
        const double maxY = centerY() + halfWidth;

        return point.x >= minX
            && point.x <= maxX
            && point.y >= minY
            && point.y <= maxY;
    }
}

bool PitchGeometry::isInsidePitch(PitchPoint point) {
    return point.x >= 0.0
        && point.x <= LengthMeters
        && point.y >= 0.0
        && point.y <= WidthMeters;
}

PitchPoint PitchGeometry::clampToPitch(PitchPoint point) {
    return PitchPoint{
        std::clamp(point.x, 0.0, LengthMeters),
        std::clamp(point.y, 0.0, WidthMeters)
    };
}

bool PitchGeometry::isInsideHomePenaltyArea(PitchPoint point) {
    return isInsideCenteredArea(point, 0.0, PenaltyAreaDepthMeters, PenaltyAreaWidthMeters);
}

bool PitchGeometry::isInsideAwayPenaltyArea(PitchPoint point) {
    return isInsideCenteredArea(
        point,
        LengthMeters - PenaltyAreaDepthMeters,
        LengthMeters,
        PenaltyAreaWidthMeters);
}

bool PitchGeometry::isInsideEitherPenaltyArea(PitchPoint point) {
    return isInsideHomePenaltyArea(point) || isInsideAwayPenaltyArea(point);
}

bool PitchGeometry::isInsideHomeSixYardBox(PitchPoint point) {
    return isInsideCenteredArea(point, 0.0, SixYardBoxDepthMeters, SixYardBoxWidthMeters);
}

bool PitchGeometry::isInsideAwaySixYardBox(PitchPoint point) {
    return isInsideCenteredArea(
        point,
        LengthMeters - SixYardBoxDepthMeters,
        LengthMeters,
        SixYardBoxWidthMeters);
}

PitchPoint PitchGeometry::homePenaltySpot() {
    return PitchPoint{ PenaltySpotDistanceMeters, centerY() };
}

PitchPoint PitchGeometry::awayPenaltySpot() {
    return PitchPoint{ LengthMeters - PenaltySpotDistanceMeters, centerY() };
}

PitchPoint PitchGeometry::homeGoalCenter() {
    return PitchPoint{ 0.0, centerY() };
}

PitchPoint PitchGeometry::awayGoalCenter() {
    return PitchPoint{ LengthMeters, centerY() };
}

double PitchGeometry::distance(PitchPoint a, PitchPoint b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}
