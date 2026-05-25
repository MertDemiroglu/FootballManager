#include"fm/match_engine/ActionSelector.h"

#include"DeterministicRandom.h"

#include<algorithm>
#include<cmath>

namespace {
    double decisionSharpness(const PlayerAttributes& attributes) {
        return 0.75 + std::clamp(attributes.mental.decisions, 0, 100) / 35.0;
    }

    double candidateWeight(const ActionCandidate& candidate, double sharpness) {
        return std::pow(std::max(candidate.finalScore, 0.1), sharpness);
    }

    bool isPassAction(BallCarrierActionType type) {
        return type == BallCarrierActionType::ShortPass
            || type == BallCarrierActionType::BackPass
            || type == BallCarrierActionType::ThroughBall
            || type == BallCarrierActionType::SwitchPlay
            || type == BallCarrierActionType::LowCross
            || type == BallCarrierActionType::HighCross
            || type == BallCarrierActionType::Cutback;
    }

    double actionSkillMultiplier(
        BallCarrierActionType type,
        const PlayerAttributes& attributes) {
        if (isPassAction(type)) {
            const double passingSkill =
                (static_cast<double>(attributes.technical.passing)
                    + static_cast<double>(attributes.mental.vision)
                    + static_cast<double>(attributes.mental.decisions)) / 3.0;
            return std::clamp(0.85 + ((passingSkill - 50.0) / 160.0), 0.78, 1.18);
        }
        if (type == BallCarrierActionType::Shoot) {
            const double shootingSkill =
                (static_cast<double>(attributes.technical.shooting)
                    + static_cast<double>(attributes.mental.composure)) / 2.0;
            return std::clamp(0.88 + ((shootingSkill - 50.0) / 180.0), 0.80, 1.16);
        }
        if (type == BallCarrierActionType::Carry
            || type == BallCarrierActionType::Dribble
            || type == BallCarrierActionType::CutInside) {
            const double carrySkill =
                (static_cast<double>(attributes.technical.dribbling)
                    + static_cast<double>(attributes.physical.agility)) / 2.0;
            return std::clamp(0.90 + ((carrySkill - 50.0) / 190.0), 0.82, 1.14);
        }

        return 1.0;
    }
}

ActionSelectionResult ActionSelector::select(const ActionSelectionRequest& request) const {
    ActionSelectionResult result;
    result.rankedCandidates = request.candidates;

    std::sort(
        result.rankedCandidates.begin(),
        result.rankedCandidates.end(),
        [](const ActionCandidate& lhs, const ActionCandidate& rhs) {
            return lhs.finalScore > rhs.finalScore;
        });

    if (result.rankedCandidates.empty()) {
        result.selected = std::nullopt;
        return result;
    }

    const double sharpness = decisionSharpness(request.decisionMakerAttributes);
    double totalWeight = 0.0;
    for (const ActionCandidate& candidate : result.rankedCandidates) {
        totalWeight += candidateWeight(candidate, sharpness)
            * actionSkillMultiplier(candidate.type, request.decisionMakerAttributes);
    }

    if (totalWeight <= 0.0) {
        result.selected = result.rankedCandidates.front();
        return result;
    }

    const double roll = matchEngineDeterministicUnitInterval(
        request.seed ^ static_cast<std::uint64_t>(result.rankedCandidates.size())) * totalWeight;

    double cursor = 0.0;
    for (const ActionCandidate& candidate : result.rankedCandidates) {
        cursor += candidateWeight(candidate, sharpness)
            * actionSkillMultiplier(candidate.type, request.decisionMakerAttributes);
        if (roll < cursor) {
            result.selected = candidate;
            return result;
        }
    }

    result.selected = result.rankedCandidates.back();
    return result;
}
