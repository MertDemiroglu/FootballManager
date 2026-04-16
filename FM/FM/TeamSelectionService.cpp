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
#include <unordered_set>
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

struct TeamSelectionIndex {
    std::array<RoleTierBuckets, static_cast<std::size_t>(FormationSlotRole::Striker) + 1> byRole{};
};

constexpr std::size_t kNaturalCapPerSlot = 3;
constexpr std::size_t kCloseCapPerSlot = 2;
constexpr std::size_t kAdaptedCapPerSlot = 1;
constexpr std::size_t kEmergencyCapPerSlot = 1;

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

std::vector<FormationSlotRole> getSupportedSlotRoles() {
    std::vector<FormationSlotRole> roles;
    roles.reserve(static_cast<std::size_t>(FormationSlotRole::Striker));

    for (int rawRole = static_cast<int>(FormationSlotRole::Goalkeeper);
         rawRole <= static_cast<int>(FormationSlotRole::Striker);
         ++rawRole) {
        const auto role = static_cast<FormationSlotRole>(rawRole);
        if (isSlotRoleSupported(role)) {
            roles.push_back(role);
        }
    }

    return roles;
}

void sortRankedCandidates(std::vector<RankedRoleCandidate>& candidates) {
    std::sort(candidates.begin(), candidates.end(), [](const RankedRoleCandidate& lhs, const RankedRoleCandidate& rhs) {
        if (!isNearlyEqual(lhs.baseSlotScore, rhs.baseSlotScore)) {
            return lhs.baseSlotScore > rhs.baseSlotScore;
        }
        return lhs.playerId < rhs.playerId;
    });
}

TeamSelectionIndex buildTeamSelectionIndex(const std::vector<PlayerCandidate>& players) {
    TeamSelectionIndex index;
    const std::vector<FormationSlotRole> supportedRoles = getSupportedSlotRoles();

    // Build role/tier buckets once per team-sheet build so formation evaluation can reuse
    // a narrow precomputed eligibility view instead of rescanning the squad per slot.
    for (const PlayerCandidate& candidate : players) {
        for (FormationSlotRole role : supportedRoles) {
            const RoleFitTier fitTier = getRoleFitTier(candidate.player->getPlayerPosition(), role);
            if (fitTier == RoleFitTier::Invalid || fitTier == RoleFitTier::SevereMismatch) {
                continue;
            }

            RankedRoleCandidate rankedCandidate{ candidate.id, calculateEffectivePowerForSlot(*candidate.player, role) };
            RoleTierBuckets& buckets = index.byRole[static_cast<std::size_t>(role)];

            switch (fitTier) {
            case RoleFitTier::Natural:
                buckets.natural.push_back(rankedCandidate);
                break;
            case RoleFitTier::Close:
                buckets.close.push_back(rankedCandidate);
                break;
            case RoleFitTier::Adapted:
                buckets.adapted.push_back(rankedCandidate);
                break;
            case RoleFitTier::Emergency:
                buckets.emergency.push_back(rankedCandidate);
                break;
            case RoleFitTier::SevereMismatch:
            case RoleFitTier::Invalid:
                break;
            }
        }
    }

    for (FormationSlotRole role : supportedRoles) {
        RoleTierBuckets& buckets = index.byRole[static_cast<std::size_t>(role)];
        sortRankedCandidates(buckets.natural);
        sortRankedCandidates(buckets.close);
        sortRankedCandidates(buckets.adapted);
        sortRankedCandidates(buckets.emergency);
    }

    return index;
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

void appendCandidatesFromTier(std::vector<SlotOption>& slotOptions,
                              std::unordered_set<PlayerId>& seenPlayerIds,
                              const std::vector<RankedRoleCandidate>& tierCandidates,
                              RoleFitTier tier,
                              std::size_t cap) {
    std::size_t added = 0;
    for (const RankedRoleCandidate& candidate : tierCandidates) {
        if (added >= cap) {
            break;
        }

        if (!seenPlayerIds.emplace(candidate.playerId).second) {
            continue;
        }

        slotOptions.push_back(SlotOption{ candidate.playerId, candidate.baseSlotScore, tier });
        ++added;
    }
}

AssignmentResult buildBestAssignmentForTemplate(const std::vector<FormationSlotRole>& templateSlots,
                                                 const TeamSelectionIndex& selectionIndex,
                                                 std::size_t slotLimit,
                                                 std::size_t squadSize) {
    const std::size_t slotCount = std::min<std::size_t>(slotLimit, templateSlots.size());
    const std::vector<FormationSlotRole> activeSlots(templateSlots.begin(), templateSlots.begin() + slotCount);

    std::array<std::size_t, static_cast<std::size_t>(FormationSlotRole::Striker) + 1> roleDemand{};
    for (FormationSlotRole role : activeSlots) {
        ++roleDemand[static_cast<std::size_t>(role)];
    }

    std::vector<std::vector<SlotOption>> slotOptions(slotCount);

    for (std::size_t slotIdx = 0; slotIdx < slotCount; ++slotIdx) {
        const FormationSlotRole slotRole = activeSlots[slotIdx];
        const RoleTierBuckets& buckets = selectionIndex.byRole[static_cast<std::size_t>(slotRole)];

        std::vector<SlotOption>& options = slotOptions[slotIdx];
        options.reserve(kNaturalCapPerSlot + kCloseCapPerSlot + kAdaptedCapPerSlot + kEmergencyCapPerSlot);

        std::unordered_set<PlayerId> seenPlayerIds;
        seenPlayerIds.reserve(kNaturalCapPerSlot + kCloseCapPerSlot + kAdaptedCapPerSlot + kEmergencyCapPerSlot);

        const std::size_t demand = roleDemand[static_cast<std::size_t>(slotRole)];
        std::size_t coverage = buckets.natural.size();

        // Tier-gated expansion: only open fallback tiers when the stronger tiers do not
        // provide enough coverage for this role demand in the current formation.
        appendCandidatesFromTier(options, seenPlayerIds, buckets.natural, RoleFitTier::Natural, kNaturalCapPerSlot);

        if (coverage < demand) {
            appendCandidatesFromTier(options, seenPlayerIds, buckets.close, RoleFitTier::Close, kCloseCapPerSlot);
            coverage += buckets.close.size();
        }

        if (coverage < demand) {
            appendCandidatesFromTier(options, seenPlayerIds, buckets.adapted, RoleFitTier::Adapted, kAdaptedCapPerSlot);
            coverage += buckets.adapted.size();
        }

        if (coverage < demand) {
            appendCandidatesFromTier(options, seenPlayerIds, buckets.emergency, RoleFitTier::Emergency, kEmergencyCapPerSlot);
        }

        std::sort(options.begin(), options.end(), [](const SlotOption& lhs, const SlotOption& rhs) {
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
    std::unordered_set<PlayerId> usedPlayerIds;
    usedPlayerIds.reserve(squadSize);

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
            if (usedPlayerIds.find(option.playerId) != usedPlayerIds.end()) {
                continue;
            }

            usedPlayerIds.emplace(option.playerId);
            currentAssigned[slotIdx] = option.playerId;
            self(self,
                 depth + 1,
                 assignedCount + 1,
                 totalPower + option.effectivePower,
                 fitScore + getRoleFitTierScore(option.fitTier));
            currentAssigned[slotIdx] = 0;
            usedPlayerIds.erase(option.playerId);
        }

        self(self, depth + 1, assignedCount, totalPower, fitScore);
    };

    backtrack(backtrack, 0, 0, 0.0, 0);
    return best;
}

FormationSelectionCandidate evaluateFormationCandidate(FormationId formation,
                                                        const TeamSelectionIndex& selectionIndex,
                                                        std::size_t starterTargetCount,
                                                        std::size_t squadSize) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    AssignmentResult assignment = buildBestAssignmentForTemplate(slotTemplate, selectionIndex, starterTargetCount, squadSize);

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

    // Phase 1 intentionally uses only static/semi-static role-adjusted power. Dynamic
    // matchday factors (form/morale/fitness/fatigue) are deferred to Phase 2.
    const TeamSelectionIndex selectionIndex = buildTeamSelectionIndex(players);

    const FormationId preferredFormation = team.getHeadCoach().getPreferredFormation();
    const std::vector<FormationId> formationOrder = buildDeterministicFormationOrder(preferredFormation);

    FormationSelectionCandidate bestCandidate{};
    bool hasBestCandidate = false;

    for (FormationId formationId : formationOrder) {
        FormationSelectionCandidate candidate = evaluateFormationCandidate(formationId, selectionIndex, starterTargetCount, players.size());

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
