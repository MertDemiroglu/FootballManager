#include"fm/match_engine/ball/ShotTargetModel.h"

#include<algorithm>
#include<cmath>

namespace {
    PitchPoint goalCenterFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::awayGoalCenter()
            : PitchGeometry::homeGoalCenter();
    }

    double clampDouble(double value, double minimum, double maximum) {
        return std::clamp(value, minimum, maximum);
    }
}

ShotTargetResult ShotTargetModel::chooseTarget(const ShotTargetContext& context) const {
    const PitchPoint goalCenter = goalCenterFor(context.attackingDirection);
    const double technique =
        context.shooterFinishing * 0.40
        + context.shooterTechnique * 0.35
        + context.shooterComposure * 0.25;
    const double distance = PitchGeometry::distance(context.shotOrigin, goalCenter);
    const double pressure = clampDouble(context.pressure, 0.0, 100.0);
    const double placementQuality = clampDouble(technique - pressure * 0.35, 0.0, 100.0);
    const double targetDifficulty = clampDouble(distance * 1.4 + pressure * 0.40, 0.0, 100.0);

    const double centerY = PitchGeometry::WidthMeters / 2.0;
    const double originLaneOffset = clampDouble((context.shotOrigin.y - centerY) / centerY, -1.0, 1.0);
    const double composedPlacement = (placementQuality - 50.0) / 50.0;
    const double targetYOffset = clampDouble(-originLaneOffset * 1.10 + composedPlacement * 0.35, -2.2, 2.2);

    return ShotTargetResult{
        PitchGeometry::clampToPitch(PitchPoint{ goalCenter.x, goalCenter.y + targetYOffset }),
        targetDifficulty,
        placementQuality
    };
}
