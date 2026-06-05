#include"fm/match_engine/ball/ShotTargetSelector.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<array>

namespace {
    double attributeOrDefault(int value) {
        const ShootingModelTuning tuning;
        return value > 0 ? std::clamp(static_cast<double>(value), 0.0, 100.0) : tuning.defaultAttribute;
    }

    double shotTypeTargetDifficulty(ShotType type) {
        switch (type) {
        case ShotType::ControlledFinish:
            return 8.0;
        case ShotType::PlacedShot:
            return 18.0;
        case ShotType::PowerShot:
            return 15.0;
        case ShotType::LongShot:
            return 30.0;
        case ShotType::TightAngleShot:
            return 34.0;
        case ShotType::DesperationShot:
            return 42.0;
        }
        return 18.0;
    }

    template<typename T, std::size_t Size>
    T weightedChoice(const std::array<std::pair<T, double>, Size>& weights, std::uint64_t seed) {
        double total = 0.0;
        for (const auto& entry : weights) {
            total += std::max(0.0, entry.second);
        }
        double roll = matchEngineDeterministicUnitInterval(seed) * std::max(total, 0.001);
        for (const auto& entry : weights) {
            roll -= std::max(0.0, entry.second);
            if (roll <= 0.0) {
                return entry.first;
            }
        }
        return weights.back().first;
    }

    double goalLineXFor(AttackingDirection direction) {
        return direction == AttackingDirection::HomeToAway
            ? PitchGeometry::LengthMeters
            : 0.0;
    }
}

PitchPoint shotTargetPointFor(
    PitchPoint shotOrigin,
    AttackingDirection attackingDirection,
    ShotTargetZone zone) {
    const double centerY = PitchGeometry::WidthMeters / 2.0;
    const double halfGoal = PitchGeometry::GoalWidthMeters / 2.0;
    const bool nearIsLowerY = shotOrigin.y < centerY;

    double laneOffset = 0.0;
    if (zone.lane == ShotTargetLane::NearPost) {
        laneOffset = nearIsLowerY ? -halfGoal * 0.72 : halfGoal * 0.72;
    } else if (zone.lane == ShotTargetLane::FarPost) {
        laneOffset = nearIsLowerY ? halfGoal * 0.72 : -halfGoal * 0.72;
    }

    return PitchPoint{ goalLineXFor(attackingDirection), centerY + laneOffset };
}

ShotTargetSelectionResult ShotTargetSelector::select(
    const ShotContext& context,
    ShotType shotType) const {
    const double shooting = attributeOrDefault(context.shooterAttributes.technical.shooting);
    const double technique = attributeOrDefault(context.shooterAttributes.technical.technique);
    const double composure = attributeOrDefault(context.shooterAttributes.mental.composure);
    const double decisions = attributeOrDefault(context.shooterAttributes.mental.decisions);
    const double targetSkill =
        (shooting * 0.30) + (technique * 0.25) + (composure * 0.25) + (decisions * 0.20);
    const double skillLift = std::clamp((targetSkill - 50.0) / 50.0, -0.7, 0.9);
    const double pressure = std::clamp(context.pressure / 100.0, 0.0, 1.0);
    const double tightAngle = std::clamp(1.0 - (context.centrality * 4.0), 0.0, 1.0);

    const std::array<std::pair<ShotTargetLane, double>, 3> laneWeights{ {
        { ShotTargetLane::NearPost, 24.0 + (tightAngle * 18.0) + (pressure * 8.0) },
        { ShotTargetLane::Center, 18.0 - (skillLift * 5.0) + (pressure * 8.0) },
        { ShotTargetLane::FarPost, 30.0 + (skillLift * 16.0) + (context.centrality * 10.0) - (tightAngle * 6.0) }
    } };
    const std::array<std::pair<ShotTargetHeight, double>, 3> heightWeights{ {
        { ShotTargetHeight::Low, 34.0 + (skillLift * 10.0) },
        { ShotTargetHeight::Mid, 24.0 + (pressure * 6.0) },
        { ShotTargetHeight::High, 15.0 + (shotType == ShotType::PowerShot ? 8.0 : 0.0) - (skillLift * 4.0) }
    } };

    ShotTargetZone zone;
    zone.lane = weightedChoice(laneWeights, context.seed ^ 0x71e133ULL);
    zone.height = weightedChoice(heightWeights, context.seed ^ 0x926b77ULL);

    const double heightDifficulty =
        zone.height == ShotTargetHeight::High ? 12.0 : (zone.height == ShotTargetHeight::Low ? 6.0 : 4.0);
    const double laneDifficulty =
        zone.lane == ShotTargetLane::Center ? 4.0 : 10.0;
    const double placementQuality = std::clamp(targetSkill - (context.pressure * 0.24), 0.0, 100.0);
    const double targetDifficulty = std::clamp(
        shotTypeTargetDifficulty(shotType)
            + laneDifficulty
            + heightDifficulty
            + (context.distanceMeters * 0.55)
            + ((1.0 - context.centrality) * 16.0),
        0.0,
        100.0);

    return ShotTargetSelectionResult{
        zone,
        shotTargetPointFor(context.shotOrigin, context.attackingDirection, zone),
        targetDifficulty,
        placementQuality
    };
}
