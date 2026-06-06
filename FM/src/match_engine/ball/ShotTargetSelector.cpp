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

    double shotTypeTargetDifficulty(ShotType type, const ShotTargetSelectionTuning& tuning) {
        switch (type) {
        case ShotType::ControlledFinish:
            return tuning.controlledFinishTargetDifficulty;
        case ShotType::PlacedShot:
            return tuning.placedShotTargetDifficulty;
        case ShotType::PowerShot:
            return tuning.powerShotTargetDifficulty;
        case ShotType::LongShot:
            return tuning.longShotTargetDifficulty;
        case ShotType::TightAngleShot:
            return tuning.tightAngleShotTargetDifficulty;
        case ShotType::DesperationShot:
            return tuning.desperationShotTargetDifficulty;
        }
        return tuning.placedShotTargetDifficulty;
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
    const ShotTargetSelectionTuning tuning;
    const bool nearIsLowerY = shotOrigin.y < centerY;

    double laneOffset = 0.0;
    if (zone.lane == ShotTargetLane::NearPost) {
        laneOffset = nearIsLowerY
            ? -halfGoal * tuning.goalLaneOffsetShare
            : halfGoal * tuning.goalLaneOffsetShare;
    } else if (zone.lane == ShotTargetLane::FarPost) {
        laneOffset = nearIsLowerY
            ? halfGoal * tuning.goalLaneOffsetShare
            : -halfGoal * tuning.goalLaneOffsetShare;
    }

    return PitchPoint{ goalLineXFor(attackingDirection), centerY + laneOffset };
}

ShotTargetSelectionResult ShotTargetSelector::select(
    const ShotContext& context,
    ShotType shotType) const {
    const ShootingModelTuning modelTuning;
    const ShotTargetSelectionTuning& tuning = modelTuning.targetSelection;
    const double shooting = attributeOrDefault(context.shooterAttributes.technical.shooting);
    const double technique = attributeOrDefault(context.shooterAttributes.technical.technique);
    const double composure = attributeOrDefault(context.shooterAttributes.mental.composure);
    const double decisions = attributeOrDefault(context.shooterAttributes.mental.decisions);
    const double targetSkill =
        (shooting * tuning.shootingSkillWeight)
        + (technique * tuning.techniqueSkillWeight)
        + (composure * tuning.composureSkillWeight)
        + (decisions * tuning.decisionsSkillWeight);
    const double skillLift = std::clamp(
        (targetSkill - tuning.skillBaseline) / tuning.skillScale,
        tuning.minimumSkillLift,
        tuning.maximumSkillLift);
    const double pressure = std::clamp(context.pressure / tuning.pressureScale, 0.0, 1.0);
    const double tightAngle =
        std::clamp(1.0 - (context.centrality * tuning.tightAngleCentralityScale), 0.0, 1.0);

    const std::array<std::pair<ShotTargetLane, double>, 3> laneWeights{ {
        { ShotTargetLane::NearPost,
            tuning.nearPostBaseWeight
                + (tightAngle * tuning.nearPostTightAngleBonus)
                + (pressure * tuning.nearPostPressureBonus) },
        { ShotTargetLane::Center,
            tuning.centerBaseWeight
                - (skillLift * tuning.centerSkillPenalty)
                + (pressure * tuning.centerPressureBonus) },
        { ShotTargetLane::FarPost,
            tuning.farPostBaseWeight
                + (skillLift * tuning.farPostSkillBonus)
                + (context.centrality * tuning.farPostCentralityBonus)
                - (tightAngle * tuning.farPostTightAnglePenalty) }
    } };
    const std::array<std::pair<ShotTargetHeight, double>, 3> heightWeights{ {
        { ShotTargetHeight::Low,
            tuning.lowHeightBaseWeight + (skillLift * tuning.lowHeightSkillBonus) },
        { ShotTargetHeight::Mid,
            tuning.midHeightBaseWeight + (pressure * tuning.midHeightPressureBonus) },
        { ShotTargetHeight::High,
            tuning.highHeightBaseWeight
                + (shotType == ShotType::PowerShot ? tuning.highHeightPowerShotBonus : 0.0)
                - (skillLift * tuning.highHeightSkillPenalty) }
    } };

    ShotTargetZone zone;
    zone.lane = weightedChoice(laneWeights, context.seed ^ 0x71e133ULL);
    zone.height = weightedChoice(heightWeights, context.seed ^ 0x926b77ULL);

    const double heightDifficulty =
        zone.height == ShotTargetHeight::High
            ? tuning.highHeightDifficulty
            : (zone.height == ShotTargetHeight::Low
                ? tuning.lowHeightDifficulty
                : tuning.midHeightDifficulty);
    const double laneDifficulty =
        zone.lane == ShotTargetLane::Center
            ? tuning.centerLaneDifficulty
            : tuning.postLaneDifficulty;
    const double placementQuality =
        std::clamp(targetSkill - (context.pressure * tuning.placementPressurePenalty), 0.0, 100.0);
    const double targetDifficulty = std::clamp(
        shotTypeTargetDifficulty(shotType, tuning)
            + laneDifficulty
            + heightDifficulty
            + (context.distanceMeters * tuning.distanceDifficultyWeight)
            + ((1.0 - context.centrality) * tuning.tightAngleDifficultyWeight),
        0.0,
        100.0);

    return ShotTargetSelectionResult{
        zone,
        shotTargetPointFor(context.shotOrigin, context.attackingDirection, zone),
        targetDifficulty,
        placementQuality
    };
}
