#include"fm/match_engine/decision/ActionSelector.h"

#include"../DeterministicRandom.h"
#include"fm/match_engine/decision/DecisionTuningProfile.h"

#include<algorithm>
#include<cmath>

namespace {
    double decisionSharpness(const PlayerAttributes& attributes) {
        const ActionSelectionTuning tuning = defaultActionSelectionTuning();
        return tuning.decisionSharpnessBase
            + std::clamp(attributes.mental.decisions, 0, 100) / tuning.decisionSharpnessFromDecisions;
    }

    double candidateWeight(const ActionCandidate& candidate, double sharpness) {
        const ActionSelectionTuning tuning = defaultActionSelectionTuning();
        return std::pow(std::max(candidate.finalScore, tuning.minimumCandidateWeightScore), sharpness);
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
        const ActionSelectionTuning tuning = defaultActionSelectionTuning();
        if (isPassAction(type)) {
            const double passingSkill =
                (static_cast<double>(attributes.technical.passing)
                    + static_cast<double>(attributes.mental.vision)
                    + static_cast<double>(attributes.mental.decisions)) / 3.0;
            return std::clamp(
                tuning.passSkillBaseline + ((passingSkill - 50.0) / tuning.passSkillInfluence),
                tuning.passSkillMinimum,
                tuning.passSkillMaximum);
        }
        if (type == BallCarrierActionType::Shoot) {
            const double shootingSkill =
                (static_cast<double>(attributes.technical.shooting)
                    + static_cast<double>(attributes.mental.composure)) / 2.0;
            return std::clamp(
                tuning.shotSkillBaseline + ((shootingSkill - 50.0) / tuning.shotSkillInfluence),
                tuning.shotSkillMinimum,
                tuning.shotSkillMaximum);
        }
        if (type == BallCarrierActionType::Carry
            || type == BallCarrierActionType::Dribble
            || type == BallCarrierActionType::CutInside) {
            const double carrySkill =
                (static_cast<double>(attributes.technical.dribbling)
                    + static_cast<double>(attributes.physical.agility)) / 2.0;
            return std::clamp(
                tuning.carrySkillBaseline + ((carrySkill - 50.0) / tuning.carrySkillInfluence),
                tuning.carrySkillMinimum,
                tuning.carrySkillMaximum);
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
