#include"fm/match_engine/ball/ShotQualityModel.h"

#include"fm/match_engine/geometry/PitchGeometry.h"

#include<algorithm>
#include<cmath>

namespace {
    constexpr double Pi = 3.14159265358979323846;

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }

    PitchPoint goalCenterFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    double goalLineXFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters
            : 0.0;
    }

    double goalAngleRadians(PitchPoint shotLocation, AttackingDirection direction) {
        const double halfGoalWidth = PitchGeometry::GoalWidthMeters / 2.0;
        const double centerY = PitchGeometry::WidthMeters / 2.0;
        const double goalX = goalLineXFor(direction);

        const PitchPoint postA{ goalX, centerY - halfGoalWidth };
        const PitchPoint postB{ goalX, centerY + halfGoalWidth };

        const double ax = postA.x - shotLocation.x;
        const double ay = postA.y - shotLocation.y;
        const double bx = postB.x - shotLocation.x;
        const double by = postB.y - shotLocation.y;

        const double lenA = std::hypot(ax, ay);
        const double lenB = std::hypot(bx, by);
        if (lenA <= 0.001 || lenB <= 0.001) {
            return Pi;
        }

        const double cosine = clampDouble(((ax * bx) + (ay * by)) / (lenA * lenB), -1.0, 1.0);
        return std::acos(cosine);
    }
}

double ShotQualityModel::calculateOpenPlayXG(
    PitchPoint shotLocation,
    AttackingDirection attackingDirection,
    double pressure) {
    const double distanceMeters =
        PitchGeometry::distance(shotLocation, goalCenterFor(attackingDirection));
    const double angleRadians = goalAngleRadians(shotLocation, attackingDirection);
    const double clampedPressure = clampDouble(pressure, 0.0, 100.0);

    const double logit =
        -1.25
        - (0.095 * distanceMeters)
        + (1.65 * angleRadians)
        - (0.010 * clampedPressure);
    const double xg = 1.0 / (1.0 + std::exp(-logit));
    return clampDouble(xg, 0.01, 0.55);
}
