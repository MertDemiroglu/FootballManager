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
struct SquadPlayerRef {
    const Footballer* player = nullptr;
    PlayerId id = 0;
};

struct PositionBucketEntry {
    const Footballer* player = nullptr;
    PlayerId id = 0;
};

struct NaturalPositionIndex {
    std::array<std::vector<PositionBucketEntry>, static_cast<std::size_t>(PlayerPosition::Striker) + 1> byPosition{};
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
    const Footballer* player = nullptr;
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

std::vector<SquadPlayerRef> collectSquadPlayers(const Team& team) {
    std::vector<SquadPlayerRef> players;
    players.reserve(team.getPlayers().size());

    for (const auto& player : team.getPlayers()) {
        if (!player) {
            continue;
        }

        players.push_back(SquadPlayerRef{ player.get(), player->getId() });
    }

    std::sort(players.begin(), players.end(), [](const SquadPlayerRef& lhs, const SquadPlayerRef& rhs) {
        return lhs.id < rhs.id;
    });

    return players;
}

NaturalPositionIndex buildNaturalPositionIndex(const std::vector<SquadPlayerRef>& squadPlayers) {
    NaturalPositionIndex index;

    // Build direct natural-position buckets once per team-sheet build; buckets store
    // pointers to existing squad players (no player-object copies).
    for (const SquadPlayerRef& playerRef : squadPlayers) {
        const PlayerPosition naturalPosition = playerRef.player->getPlayerPosition();
        if (!isKnownPlayerPosition(naturalPosition)) {
            continue;
        }

        index.byPosition[static_cast<std::size_t>(naturalPosition)].push_back(PositionBucketEntry{ playerRef.player, playerRef.id });
    }

    for (std::size_t posIndex = static_cast<std::size_t>(PlayerPosition::Goalkeeper);
         posIndex <= static_cast<std::size_t>(PlayerPosition::Striker);
         ++posIndex) {
        std::vector<PositionBucketEntry>& bucket = index.byPosition[posIndex];
        std::sort(bucket.begin(), bucket.end(), [](const PositionBucketEntry& lhs, const PositionBucketEntry& rhs) {
            return lhs.id < rhs.id;
        });
    }

    return index;
}

void appendBucketSources(FormationSlotRole slotRole, RoleFitTier tier, std::vector<PlayerPosition>& outSources) {
    outSources.clear();

    switch (slotRole) {
    case FormationSlotRole::Goalkeeper:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::Goalkeeper);
        break;

    case FormationSlotRole::CenterBack:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::CenterBack);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::DefensiveMidfielder);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::RightBack);
        }
        if (tier == RoleFitTier::Emergency) outSources.push_back(PlayerPosition::CentralMidfielder);
        break;

    case FormationSlotRole::LeftBack:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::LeftBack);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::CenterBack);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::RightBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::RightBack:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::RightBack);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::CenterBack);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::LeftWingBack:
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::LeftWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
        }
        break;

    case FormationSlotRole::RightWingBack:
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::RightBack);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
        }
        break;

    case FormationSlotRole::DefensiveMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::DefensiveMidfielder);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::CentralMidfielder);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Emergency) outSources.push_back(PlayerPosition::AttackingMidfielder);
        break;

    case FormationSlotRole::CentralMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::CentralMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::CenterBack);
            outSources.push_back(PlayerPosition::Striker);
        }
        break;

    case FormationSlotRole::AttackingMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::AttackingMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::CentralMidfielder);
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Adapted) outSources.push_back(PlayerPosition::Striker);
        if (tier == RoleFitTier::Emergency) outSources.push_back(PlayerPosition::DefensiveMidfielder);
        break;

    case FormationSlotRole::LeftMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::LeftMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::CentralMidfielder);
            outSources.push_back(PlayerPosition::LeftWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftBack);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        break;

    case FormationSlotRole::RightMidfielder:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::RightMidfielder);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::CentralMidfielder);
            outSources.push_back(PlayerPosition::RightWinger);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightBack);
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::DefensiveMidfielder);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::LeftWinger);
        }
        break;

    case FormationSlotRole::LeftWinger:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::LeftWinger);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::LeftMidfielder);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::Striker);
            outSources.push_back(PlayerPosition::LeftBack);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::RightMidfielder);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::RightWinger:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::RightWinger);
        if (tier == RoleFitTier::Close) {
            outSources.push_back(PlayerPosition::AttackingMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::Striker);
            outSources.push_back(PlayerPosition::RightBack);
        }
        if (tier == RoleFitTier::Emergency) {
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::CentralMidfielder);
        }
        break;

    case FormationSlotRole::Striker:
        if (tier == RoleFitTier::Natural) outSources.push_back(PlayerPosition::Striker);
        if (tier == RoleFitTier::Close) outSources.push_back(PlayerPosition::AttackingMidfielder);
        if (tier == RoleFitTier::Adapted) {
            outSources.push_back(PlayerPosition::LeftWinger);
            outSources.push_back(PlayerPosition::RightWinger);
            outSources.push_back(PlayerPosition::LeftMidfielder);
            outSources.push_back(PlayerPosition::RightMidfielder);
        }
        break;

    case FormationSlotRole::Unknown:
        break;
    }
}

void collectTierCandidatesForRole(const NaturalPositionIndex& positionIndex,
                                  FormationSlotRole slotRole,
                                  RoleFitTier targetTier,
                                  std::vector<RankedRoleCandidate>& outCandidates) {
    outCandidates.clear();

    std::vector<PlayerPosition> sourcePositions;
    appendBucketSources(slotRole, targetTier, sourcePositions);

    for (PlayerPosition sourcePosition : sourcePositions) {
        const std::vector<PositionBucketEntry>& sourceBucket = positionIndex.byPosition[static_cast<std::size_t>(sourcePosition)];
        for (const PositionBucketEntry& candidateRef : sourceBucket) {
            const RoleFitTier actualTier = getRoleFitTier(candidateRef.player->getPlayerPosition(), slotRole);
            if (actualTier != targetTier) {
                continue;
            }

            outCandidates.push_back(RankedRoleCandidate{
                candidateRef.player,
                candidateRef.id,
                calculateEffectivePowerForSlot(*candidateRef.player, slotRole)
            });
        }
    }

    std::sort(outCandidates.begin(), outCandidates.end(), [](const RankedRoleCandidate& lhs, const RankedRoleCandidate& rhs) {
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

    // Demand-aware tier gating: only open lower tiers when higher tiers cannot
    // cover this role demand for the current formation.
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
                                         const NaturalPositionIndex& positionIndex) {
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

    // Prepare only active roles for this formation; roles not used by this formation
    // are not touched.
    for (FormationRoleEntry& entry : rolePlan.roleEntries) {
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Natural, entry.tierBuckets.natural);
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Close, entry.tierBuckets.close);
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Adapted, entry.tierBuckets.adapted);
        collectTierCandidatesForRole(positionIndex, entry.role, RoleFitTier::Emergency, entry.tierBuckets.emergency);

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

    // Reuse shared role-level candidate pools for duplicate slots of the same role.
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

    // Scarcity-first internal solving order.
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
                                                        const NaturalPositionIndex& positionIndex,
                                                        std::size_t starterTargetCount,
                                                        std::size_t squadSize) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    const FormationRolePlan rolePlan = buildFormationRolePlan(slotTemplate, starterTargetCount, positionIndex);
    AssignmentResult assignment = buildBestAssignmentForRolePlan(rolePlan, squadSize);

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
    const std::vector<SquadPlayerRef> squadPlayers = collectSquadPlayers(team);
    const std::size_t starterTargetCount = std::min<std::size_t>(11, squadPlayers.size());

    // Phase 1/final hardening intentionally uses only static role-adjusted power.
    // Dynamic matchday factors (form/morale/fitness/fatigue) are deferred to Phase 2.
    const NaturalPositionIndex naturalPositionIndex = buildNaturalPositionIndex(squadPlayers);

    const FormationId preferredFormation = team.getHeadCoach().getPreferredFormation();
    const std::vector<FormationId> formationOrder = buildDeterministicFormationOrder(preferredFormation);

    FormationSelectionCandidate bestCandidate{};
    bool hasBestCandidate = false;

    for (FormationId formationId : formationOrder) {
        FormationSelectionCandidate candidate = evaluateFormationCandidate(formationId,
                                                                           naturalPositionIndex,
                                                                           starterTargetCount,
                                                                           squadPlayers.size());

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
