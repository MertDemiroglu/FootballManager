#include "TeamSelectionService.h"

#include "Footballer.h"
#include "Formation.h"
#include "PlayerRoleFit.h"
#include "Team.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace {
struct PlayerCandidate {
    const Footballer* player = nullptr;
    PlayerId id = 0;
};

struct SlotOption {
    PlayerId playerId = 0;
    double effectivePower = 0.0;
    RoleFitTier fitTier = RoleFitTier::Invalid;
};

struct AssignmentResult {
    std::vector<PlayerId> orderedPlayerIdsByTemplate;
    int assignedSlotCount = 0;
    double totalEffectivePower = 0.0;
    int totalFitScore = 0;
};

struct FormationSelectionCandidate {
    FormationId formation = FormationId::FourFourTwo;
    std::vector<TeamSheetSlotAssignment> orderedAssignments;
    int assignedSlotCount = 0;
    double totalEffectivePower = 0.0;
    int totalFitScore = 0;
    bool fullySatisfied = false;
};

struct RankedRoleCandidate {
    PlayerId playerId = 0;
    double baseSlotScore = 0.0;
};

struct RoleTierBuckets {
    std::vector<RankedRoleCandidate> natural;
    std::vector<RankedRoleCandidate> close;
    std::vector<RankedRoleCandidate> adapted;
    std::vector<RankedRoleCandidate> emergency;
};

struct FormationRoleEntry {
    FormationSlotRole role = FormationSlotRole::Unknown;
    std::size_t demand = 0;
    RoleTierBuckets tierBuckets;
    std::vector<SlotOption> expandedOptions;
};

struct FormationRolePlan {
    std::vector<FormationSlotRole> activeSlots;
    std::vector<FormationRoleEntry> roleEntries;
    std::array<int, static_cast<std::size_t>(FormationSlotRole::Striker) + 1> roleToEntryIndex{};

    FormationRolePlan() {
        roleToEntryIndex.fill(-1);
    }
};

bool isNearlyEqual(double lhs, double rhs) {
    return std::fabs(lhs - rhs) < 1e-9;
}

bool lexicographicallyBetterIds(const std::vector<PlayerId>& lhs, const std::vector<PlayerId>& rhs) {
    const std::size_t size = std::min(lhs.size(), rhs.size());
    for (std::size_t i = 0; i < size; ++i) {
        const PlayerId leftId = lhs[i] == 0 ? std::numeric_limits<PlayerId>::max() : lhs[i];
        const PlayerId rightId = rhs[i] == 0 ? std::numeric_limits<PlayerId>::max() : rhs[i];
        if (leftId < rightId) {
            return true;
        }
        if (leftId > rightId) {
            return false;
        }
    }

    return lhs.size() < rhs.size();
}

bool isBetterAssignment(const AssignmentResult& lhs, const AssignmentResult& rhs) {
    if (lhs.assignedSlotCount != rhs.assignedSlotCount) {
        return lhs.assignedSlotCount > rhs.assignedSlotCount;
    }

    if (!isNearlyEqual(lhs.totalEffectivePower, rhs.totalEffectivePower)) {
        return lhs.totalEffectivePower > rhs.totalEffectivePower;
    }

    if (lhs.totalFitScore != rhs.totalFitScore) {
        return lhs.totalFitScore > rhs.totalFitScore;
    }

    return lexicographicallyBetterIds(lhs.orderedPlayerIdsByTemplate, rhs.orderedPlayerIdsByTemplate);
}

bool isBetterFormationCandidate(const FormationSelectionCandidate& lhs, const FormationSelectionCandidate& rhs) {
    if (lhs.assignedSlotCount != rhs.assignedSlotCount) {
        return lhs.assignedSlotCount > rhs.assignedSlotCount;
    }

    if (!isNearlyEqual(lhs.totalEffectivePower, rhs.totalEffectivePower)) {
        return lhs.totalEffectivePower > rhs.totalEffectivePower;
    }

    if (lhs.totalFitScore != rhs.totalFitScore) {
        return lhs.totalFitScore > rhs.totalFitScore;
    }

    std::vector<PlayerId> lhsIds;
    lhsIds.reserve(lhs.orderedAssignments.size());
    for (const TeamSheetSlotAssignment& assignment : lhs.orderedAssignments) {
        lhsIds.push_back(assignment.playerId);
    }

    std::vector<PlayerId> rhsIds;
    rhsIds.reserve(rhs.orderedAssignments.size());
    for (const TeamSheetSlotAssignment& assignment : rhs.orderedAssignments) {
        rhsIds.push_back(assignment.playerId);
    }

    return lexicographicallyBetterIds(lhsIds, rhsIds);
}

std::vector<PlayerCandidate> collectCandidates(const Team& team) {
    std::vector<PlayerCandidate> candidates;
    candidates.reserve(team.getPlayers().size());

    for (const auto& player : team.getPlayers()) {
        if (!player) {
            continue;
        }

        candidates.push_back(PlayerCandidate{ player.get(), player->getId() });
    }

    std::sort(candidates.begin(), candidates.end(), [](const PlayerCandidate& lhs, const PlayerCandidate& rhs) {
        return lhs.id < rhs.id;
    });

    return candidates;
}

void sortRankedCandidates(std::vector<RankedRoleCandidate>& candidates) {
    std::sort(candidates.begin(), candidates.end(), [](const RankedRoleCandidate& lhs, const RankedRoleCandidate& rhs) {
        if (!isNearlyEqual(lhs.baseSlotScore, rhs.baseSlotScore)) {
            return lhs.baseSlotScore > rhs.baseSlotScore;
        }
        return lhs.playerId < rhs.playerId;
    });
}

void appendUniqueCandidates(std::vector<SlotOption>& options,
                            const std::vector<RankedRoleCandidate>& tierCandidates,
                            RoleFitTier tier,
                            std::size_t cap) {
    std::size_t added = 0;
    for (const RankedRoleCandidate& candidate : tierCandidates) {
        if (added >= cap) {
            break;
        }

        const bool alreadyPresent = std::any_of(options.begin(), options.end(), [&](const SlotOption& existing) {
            return existing.playerId == candidate.playerId;
        });
        if (alreadyPresent) {
            continue;
        }

        options.push_back(SlotOption{ candidate.playerId, candidate.baseSlotScore, tier });
        ++added;
    }
}

void buildDemandAwareExpandedOptions(FormationRoleEntry& entry) {
    entry.expandedOptions.clear();
    entry.expandedOptions.reserve(entry.demand + 2);

    // Demand-aware gated expansion per role:
    // - Natural gets demand + small cushion.
    // - Close/Adapted only open if previous tiers are insufficient.
    // - Emergency is at most one final fallback.
    const std::size_t naturalCap = std::min(entry.tierBuckets.natural.size(), entry.demand + 1);
    appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.natural, RoleFitTier::Natural, naturalCap);

    std::size_t covered = entry.expandedOptions.size();
    if (covered < entry.demand) {
        const std::size_t deficit = entry.demand - covered;
        const std::size_t closeCap = std::min(entry.tierBuckets.close.size(), deficit + 1);
        appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.close, RoleFitTier::Close, closeCap);
        covered = entry.expandedOptions.size();
    }

    if (covered < entry.demand) {
        const std::size_t deficit = entry.demand - covered;
        const std::size_t adaptedCap = std::min(entry.tierBuckets.adapted.size(), deficit);
        appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.adapted, RoleFitTier::Adapted, adaptedCap);
        covered = entry.expandedOptions.size();
    }

    if (covered < entry.demand) {
        const std::size_t emergencyCap = std::min<std::size_t>(entry.tierBuckets.emergency.size(), 1);
        appendUniqueCandidates(entry.expandedOptions, entry.tierBuckets.emergency, RoleFitTier::Emergency, emergencyCap);
    }

    std::sort(entry.expandedOptions.begin(), entry.expandedOptions.end(), [](const SlotOption& lhs, const SlotOption& rhs) {
        if (!isNearlyEqual(lhs.effectivePower, rhs.effectivePower)) {
            return lhs.effectivePower > rhs.effectivePower;
        }

        const int lhsTierScore = getRoleFitTierScore(lhs.fitTier);
        const int rhsTierScore = getRoleFitTierScore(rhs.fitTier);
        if (lhsTierScore != rhsTierScore) {
            return lhsTierScore > rhsTierScore;
        }

        return lhs.playerId < rhs.playerId;
    });
}

FormationRolePlan buildFormationRolePlan(const std::vector<FormationSlotRole>& templateSlots,
                                         std::size_t slotLimit,
                                         const std::vector<PlayerCandidate>& players) {
    FormationRolePlan rolePlan;
    const std::size_t slotCount = std::min<std::size_t>(slotLimit, templateSlots.size());
    rolePlan.activeSlots.assign(templateSlots.begin(), templateSlots.begin() + slotCount);

    for (FormationSlotRole slotRole : rolePlan.activeSlots) {
        const std::size_t roleIndex = static_cast<std::size_t>(slotRole);
        int& roleEntryIndex = rolePlan.roleToEntryIndex[roleIndex];
        if (roleEntryIndex == -1) {
            roleEntryIndex = static_cast<int>(rolePlan.roleEntries.size());
            rolePlan.roleEntries.push_back(FormationRoleEntry{ slotRole, 1, {}, {} });
        } else {
            ++rolePlan.roleEntries[static_cast<std::size_t>(roleEntryIndex)].demand;
        }
    }

    // Build and sort buckets only for roles that are actually present in this formation.
    for (FormationRoleEntry& entry : rolePlan.roleEntries) {
        for (const PlayerCandidate& player : players) {
            const RoleFitTier fitTier = getRoleFitTier(player.player->getPlayerPosition(), entry.role);
            if (fitTier == RoleFitTier::Invalid || fitTier == RoleFitTier::SevereMismatch) {
                continue;
            }

            RankedRoleCandidate rankedCandidate{ player.id, calculateEffectivePowerForSlot(*player.player, entry.role) };
            switch (fitTier) {
            case RoleFitTier::Natural:
                entry.tierBuckets.natural.push_back(rankedCandidate);
                break;
            case RoleFitTier::Close:
                entry.tierBuckets.close.push_back(rankedCandidate);
                break;
            case RoleFitTier::Adapted:
                entry.tierBuckets.adapted.push_back(rankedCandidate);
                break;
            case RoleFitTier::Emergency:
                entry.tierBuckets.emergency.push_back(rankedCandidate);
                break;
            case RoleFitTier::SevereMismatch:
            case RoleFitTier::Invalid:
                break;
            }
        }

        sortRankedCandidates(entry.tierBuckets.natural);
        sortRankedCandidates(entry.tierBuckets.close);
        sortRankedCandidates(entry.tierBuckets.adapted);
        sortRankedCandidates(entry.tierBuckets.emergency);

        buildDemandAwareExpandedOptions(entry);
    }

    return rolePlan;
}

std::vector<FormationId> buildDeterministicFormationOrder(FormationId preferredFormation) {
    std::vector<FormationId> orderedFormations;
    orderedFormations.reserve(getSupportedFormationIds().size());

    if (isFormationSupported(preferredFormation)) {
        orderedFormations.push_back(preferredFormation);
    }

    for (FormationId formationId : getSupportedFormationIds()) {
        if (formationId == preferredFormation) {
            continue;
        }

        orderedFormations.push_back(formationId);
    }

    return orderedFormations;
}

AssignmentResult buildBestAssignmentForRolePlan(const FormationRolePlan& rolePlan,
                                                std::size_t squadSize) {
    const std::size_t slotCount = rolePlan.activeSlots.size();
    std::vector<std::vector<SlotOption>> slotOptions(slotCount);

    // Reuse per-role expanded pools for duplicate slots (e.g. 2x CB) instead of
    // rebuilding identical candidate work for each slot.
    for (std::size_t slotIdx = 0; slotIdx < slotCount; ++slotIdx) {
        const FormationSlotRole slotRole = rolePlan.activeSlots[slotIdx];
        const int roleEntryIndex = rolePlan.roleToEntryIndex[static_cast<std::size_t>(slotRole)];
        if (roleEntryIndex < 0) {
            continue;
        }

        slotOptions[slotIdx] = rolePlan.roleEntries[static_cast<std::size_t>(roleEntryIndex)].expandedOptions;
    }

    std::vector<std::size_t> orderedSlotIndices(slotCount);
    std::iota(orderedSlotIndices.begin(), orderedSlotIndices.end(), 0);

    // Scarcity-first order dramatically reduces backtracking branching while keeping
    // final output in original template order.
    std::sort(orderedSlotIndices.begin(), orderedSlotIndices.end(), [&slotOptions](std::size_t lhs, std::size_t rhs) {
        if (slotOptions[lhs].size() != slotOptions[rhs].size()) {
            return slotOptions[lhs].size() < slotOptions[rhs].size();
        }
        return lhs < rhs;
    });

    std::vector<double> optimisticBestByDepth(slotCount, 0.0);
    for (std::size_t depth = 0; depth < orderedSlotIndices.size(); ++depth) {
        const std::size_t slotIdx = orderedSlotIndices[depth];
        if (!slotOptions[slotIdx].empty()) {
            optimisticBestByDepth[depth] = slotOptions[slotIdx].front().effectivePower;
        }
    }

    std::vector<double> optimisticSuffix(slotCount + 1, 0.0);
    for (std::size_t depth = slotCount; depth > 0; --depth) {
        optimisticSuffix[depth - 1] = optimisticSuffix[depth] + optimisticBestByDepth[depth - 1];
    }

    AssignmentResult best;
    best.orderedPlayerIdsByTemplate.assign(slotCount, 0);

    std::vector<PlayerId> currentAssigned(slotCount, 0);
    std::vector<PlayerId> usedPlayerIds;
    usedPlayerIds.reserve(slotCount);

    const auto backtrack = [&](auto&& self, std::size_t depth, int assignedCount, double totalPower, int fitScore) -> void {
        if (depth == orderedSlotIndices.size()) {
            AssignmentResult current;
            current.orderedPlayerIdsByTemplate = currentAssigned;
            current.assignedSlotCount = assignedCount;
            current.totalEffectivePower = totalPower;
            current.totalFitScore = fitScore;

            if (isBetterAssignment(current, best)) {
                best = std::move(current);
            }
            return;
        }

        const int remainingSlots = static_cast<int>(orderedSlotIndices.size() - depth);
        const int remainingPlayers = static_cast<int>(squadSize - usedPlayerIds.size());
        const int maxAdditionalAssignments = std::min(remainingSlots, remainingPlayers);
        if (assignedCount + maxAdditionalAssignments < best.assignedSlotCount) {
            return;
        }

        const double optimisticTotalPower = totalPower + optimisticSuffix[depth];
        if (assignedCount + maxAdditionalAssignments == best.assignedSlotCount && optimisticTotalPower + 1e-9 < best.totalEffectivePower) {
            return;
        }

        const std::size_t slotIdx = orderedSlotIndices[depth];
        for (const SlotOption& option : slotOptions[slotIdx]) {
            const bool alreadyUsed = std::find(usedPlayerIds.begin(), usedPlayerIds.end(), option.playerId) != usedPlayerIds.end();
            if (alreadyUsed) {
                continue;
            }

            usedPlayerIds.push_back(option.playerId);
            currentAssigned[slotIdx] = option.playerId;
            self(self,
                 depth + 1,
                 assignedCount + 1,
                 totalPower + option.effectivePower,
                 fitScore + getRoleFitTierScore(option.fitTier));
            currentAssigned[slotIdx] = 0;
            usedPlayerIds.pop_back();
        }

        self(self, depth + 1, assignedCount, totalPower, fitScore);
    };

    backtrack(backtrack, 0, 0, 0.0, 0);
    return best;
}

FormationSelectionCandidate evaluateFormationCandidate(FormationId formation,
                                                        const std::vector<PlayerCandidate>& players,
                                                        std::size_t starterTargetCount) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    const FormationRolePlan rolePlan = buildFormationRolePlan(slotTemplate, starterTargetCount, players);
    AssignmentResult assignment = buildBestAssignmentForRolePlan(rolePlan, players.size());

    FormationSelectionCandidate candidate;
    candidate.formation = formation;
    candidate.assignedSlotCount = assignment.assignedSlotCount;
    candidate.totalEffectivePower = assignment.totalEffectivePower;
    candidate.totalFitScore = assignment.totalFitScore;

    const std::size_t slotCount = assignment.orderedPlayerIdsByTemplate.size();
    candidate.orderedAssignments.reserve(slotCount);
    for (std::size_t i = 0; i < slotCount; ++i) {
        const PlayerId playerId = assignment.orderedPlayerIdsByTemplate[i];
        if (playerId == 0) {
            continue;
        }

        candidate.orderedAssignments.push_back(TeamSheetSlotAssignment{ slotTemplate[i], playerId });
    }

    candidate.fullySatisfied = candidate.assignedSlotCount == static_cast<int>(slotCount);
    return candidate;
}
} // namespace

TeamSheet TeamSelectionService::buildTeamSheet(const Team& team) const {
    const std::vector<PlayerCandidate> players = collectCandidates(team);
    const std::size_t starterTargetCount = std::min<std::size_t>(11, players.size());

    // Phase 1/Hardening intentionally uses only static role-adjusted power. Dynamic
    // matchday factors (form/morale/fitness/fatigue) are deferred to Phase 2.
    const FormationId preferredFormation = team.getHeadCoach().getPreferredFormation();
    const std::vector<FormationId> formationOrder = buildDeterministicFormationOrder(preferredFormation);

    FormationSelectionCandidate bestCandidate{};
    bool hasBestCandidate = false;

    for (FormationId formationId : formationOrder) {
        FormationSelectionCandidate candidate = evaluateFormationCandidate(formationId, players, starterTargetCount);

        if (candidate.fullySatisfied) {
            std::vector<PlayerId> starterIds;
            starterIds.reserve(candidate.orderedAssignments.size());
            for (const TeamSheetSlotAssignment& assignment : candidate.orderedAssignments) {
                starterIds.push_back(assignment.playerId);
            }

            TeamSheet teamSheet{ team.getId(), team.getHeadCoach().getId(), candidate.formation, candidate.orderedAssignments, starterIds };
            validateTeamSheetForTeam(teamSheet, team);
            return teamSheet;
        }

        if (!hasBestCandidate || isBetterFormationCandidate(candidate, bestCandidate)) {
            bestCandidate = std::move(candidate);
            hasBestCandidate = true;
        }
    }

    if (!hasBestCandidate) {
        bestCandidate.formation = isFormationSupported(preferredFormation)
            ? preferredFormation
            : getSupportedFormationIds().front();
    }

    std::vector<PlayerId> starterIds;
    starterIds.reserve(bestCandidate.orderedAssignments.size());
    for (const TeamSheetSlotAssignment& assignment : bestCandidate.orderedAssignments) {
        starterIds.push_back(assignment.playerId);
    }

    TeamSheet teamSheet{ team.getId(), team.getHeadCoach().getId(), bestCandidate.formation, bestCandidate.orderedAssignments, starterIds };
    validateTeamSheetForTeam(teamSheet, team);
    return teamSheet;
}
