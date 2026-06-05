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

    double typeDifficulty(ShotType type) {
        switch (type) {
        case ShotType::ControlledFinish:
            return 12.0;
        case ShotType::PlacedShot:
            return 18.0;
        case ShotType::PowerShot:
            return 24.0;
        case ShotType::LongShot:
            return 34.0;
        case ShotType::TightAngleShot:
            return 38.0;
        case ShotType::DesperationShot:
            return 48.0;
        }
        return 24.0;
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
    const ShootingModelTuning tuning;
    const double decisions = attributeOrDefault(context.shooterAttributes.mental.decisions);
    const double shooting = attributeOrDefault(context.shooterAttributes.technical.shooting);
    const double technique = attributeOrDefault(context.shooterAttributes.technical.technique);
    const double composure = attributeOrDefault(context.shooterAttributes.mental.composure);
    const double skill = (shooting * 0.35) + (technique * 0.28) + (composure * 0.22) + (decisions * 0.15);
    const double skillLift = std::clamp((skill - 50.0) / 50.0, -0.6, 0.8);
    const double pressure = std::clamp(context.pressure / 100.0, 0.0, 1.0);
    const double longDistance = context.distanceMeters > tuning.longShotDistance ? 1.0 : 0.0;
    const double veryLongDistance = context.distanceMeters > tuning.veryLongShotDistance ? 1.0 : 0.0;
    const double tightAngle = std::clamp(1.0 - (context.centrality * 4.2), 0.0, 1.0);
    const double closeCentral = context.distanceMeters < tuning.closeShotDistance && context.centrality > 0.22 ? 1.0 : 0.0;

    const std::array<std::pair<ShotType, double>, 6> weights{ {
        { ShotType::ControlledFinish, 28.0 + (closeCentral * 22.0) + (skillLift * 8.0) - (pressure * 8.0) },
        { ShotType::PlacedShot, 26.0 + (skillLift * 12.0) + (context.centrality * 12.0) },
        { ShotType::PowerShot, 18.0 + (pressure * 8.0) + (skillLift * 5.0) },
        { ShotType::LongShot, 4.0 + (longDistance * 30.0) + (veryLongDistance * 16.0) },
        { ShotType::TightAngleShot, 2.0 + (tightAngle * 36.0) },
        { ShotType::DesperationShot, 2.0 + (pressure * 12.0) + (veryLongDistance * 10.0) + (context.lanePressure * 0.08) }
    } };

    const ShotType selected = weightedChoice(weights, context.seed ^ 0x5a0719ULL);
    return ShotTypeSelectionResult{ selected, typeDifficulty(selected) };
}
