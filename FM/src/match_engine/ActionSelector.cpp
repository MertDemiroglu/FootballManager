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
        totalWeight += candidateWeight(candidate, sharpness);
    }

    if (totalWeight <= 0.0) {
        result.selected = result.rankedCandidates.front();
        return result;
    }

    const double roll = matchEngineDeterministicUnitInterval(
        request.seed ^ static_cast<std::uint64_t>(result.rankedCandidates.size())) * totalWeight;

    double cursor = 0.0;
    for (const ActionCandidate& candidate : result.rankedCandidates) {
        cursor += candidateWeight(candidate, sharpness);
        if (roll < cursor) {
            result.selected = candidate;
            return result;
        }
    }

    result.selected = result.rankedCandidates.back();
    return result;
}
