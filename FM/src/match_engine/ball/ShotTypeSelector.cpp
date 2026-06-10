#include"fm/match_engine/ball/ShotTypeSelector.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<array>

namespace {
    double attributeOrDefault(int value) {
        const ShootingModelTuning tuning;
        return value > 0 ? std::clamp(static_cast<double>(value), 0.0, 100.0) : tuning.defaultAttribute;
    }

    double typeDifficulty(ShotType type, const ShotTypeSelectionTuning& tuning) {
        switch (type) {
        case ShotType::ControlledFinish:
            return tuning.controlledFinishDifficulty;
        case ShotType::PlacedShot:
            return tuning.placedShotDifficulty;
        case ShotType::PowerShot:
            return tuning.powerShotDifficulty;
        case ShotType::LongShot:
            return tuning.longShotDifficulty;
        case ShotType::TightAngleShot:
            return tuning.tightAngleShotDifficulty;
        case ShotType::DesperationShot:
            return tuning.desperationShotDifficulty;
        }
        return tuning.powerShotDifficulty;
    }

    ShotType weightedChoice(const std::array<std::pair<ShotType, double>, 6>& weights, std::uint64_t seed) {
        double total = 0.0;
        for (const auto& entry : weights) {
            total += std::max(0.0, entry.second);
        }
        if (total <= 0.0) {
            return ShotType::ControlledFinish;
        }

        double roll = matchEngineDeterministicUnitInterval(seed) * total;
        for (const auto& entry : weights) {
            roll -= std::max(0.0, entry.second);
            if (roll <= 0.0) {
                return entry.first;
            }
        }
        return weights.back().first;
    }
}

ShotTypeSelectionResult ShotTypeSelector::select(const ShotContext& context) const {
    const ShootingModelTuning modelTuning;
    const ShotTypeSelectionTuning& tuning = modelTuning.typeSelection;
    const double decisions = attributeOrDefault(context.shooterAttributes.mental.decisions);
    const double shooting = attributeOrDefault(context.shooterAttributes.technical.shooting);
    const double technique = attributeOrDefault(context.shooterAttributes.technical.technique);
    const double composure = attributeOrDefault(context.shooterAttributes.mental.composure);
    const double skill =
        (shooting * tuning.shootingSkillWeight)
        + (technique * tuning.techniqueSkillWeight)
        + (composure * tuning.composureSkillWeight)
        + (decisions * tuning.decisionsSkillWeight);
    const double skillLift = std::clamp(
        (skill - tuning.skillBaseline) / tuning.skillScale,
        tuning.minimumSkillLift,
        tuning.maximumSkillLift);
    const double pressure = std::clamp(context.pressure / tuning.pressureScale, 0.0, 1.0);
    const double longDistance = context.distanceMeters > tuning.longShotDistance ? 1.0 : 0.0;
    const double veryLongDistance = context.distanceMeters > tuning.veryLongShotDistance ? 1.0 : 0.0;
    const double tightAngle =
        std::clamp(1.0 - (context.centrality * tuning.tightAngleCentralityScale), 0.0, 1.0);
    const double closeCentral =
        context.distanceMeters < tuning.closeShotDistance
            && context.centrality > tuning.closeCentralMinimumCentrality
        ? 1.0
        : 0.0;

    const std::array<std::pair<ShotType, double>, 6> weights{ {
        { ShotType::ControlledFinish,
            tuning.controlledFinishBaseWeight
                + (closeCentral * tuning.controlledFinishCloseCentralBonus)
                + (skillLift * tuning.controlledFinishSkillBonus)
                - (pressure * tuning.controlledFinishPressurePenalty) },
        { ShotType::PlacedShot,
            tuning.placedShotBaseWeight
                + (skillLift * tuning.placedShotSkillBonus)
                + (context.centrality * tuning.placedShotCentralityBonus) },
        { ShotType::PowerShot,
            tuning.powerShotBaseWeight
                + (pressure * tuning.powerShotPressureBonus)
                + (skillLift * tuning.powerShotSkillBonus) },
        { ShotType::LongShot,
            tuning.longShotBaseWeight
                + (longDistance * tuning.longShotDistanceBonus)
                + (veryLongDistance * tuning.veryLongShotDistanceBonus) },
        { ShotType::TightAngleShot,
            tuning.tightAngleShotBaseWeight
                + (tightAngle * tuning.tightAngleShotTightAngleBonus) },
        { ShotType::DesperationShot,
            tuning.desperationShotBaseWeight
                + (pressure * tuning.desperationShotPressureBonus)
                + (veryLongDistance * tuning.desperationShotVeryLongBonus)
                + (context.lanePressure * tuning.desperationShotLanePressureBonus) }
    } };

    const ShotType selected = weightedChoice(weights, context.seed ^ 0x5a0719ULL);
    return ShotTypeSelectionResult{ selected, typeDifficulty(selected, tuning) };
}
