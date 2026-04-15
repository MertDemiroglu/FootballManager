#include "TeamSelectionService.h"

#include "Footballer.h"
#include "Formation.h"
#include "PlayerRoleFit.h"
#include "Team.h"

#include <algorithm>
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

AssignmentResult buildBestAssignmentForTemplate(const std::vector<FormationSlotRole>& templateSlots,
                                                 const std::vector<PlayerCandidate>& players,
                                                 std::size_t slotLimit) {
    const std::size_t slotCount = std::min<std::size_t>(slotLimit, templateSlots.size());
    const std::vector<FormationSlotRole> activeSlots(templateSlots.begin(), templateSlots.begin() + slotCount);

    std::vector<std::vector<SlotOption>> slotOptions(slotCount);
    std::vector<double> optimisticPerSlot(slotCount, 0.0);

    for (std::size_t slotIdx = 0; slotIdx < slotCount; ++slotIdx) {
        const FormationSlotRole slotRole = activeSlots[slotIdx];
        std::vector<SlotOption>& options = slotOptions[slotIdx];
        options.reserve(players.size());

        for (const PlayerCandidate& candidate : players) {
            const RoleFitTier fitTier = getRoleFitTier(candidate.player->getPlayerPosition(), slotRole);
            if (fitTier == RoleFitTier::Invalid) {
                continue;
            }

            options.push_back(SlotOption{
                candidate.id,
                calculateEffectivePowerForSlot(*candidate.player, slotRole),
                fitTier
            });
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

        if (!options.empty()) {
            optimisticPerSlot[slotIdx] = options.front().effectivePower;
        }
    }

    std::vector<double> optimisticSuffix(slotCount + 1, 0.0);
    for (std::size_t i = slotCount; i > 0; --i) {
        optimisticSuffix[i - 1] = optimisticSuffix[i] + optimisticPerSlot[i - 1];
    }

    std::vector<std::size_t> orderedSlotIndices(slotCount);
    std::iota(orderedSlotIndices.begin(), orderedSlotIndices.end(), 0);
    std::sort(orderedSlotIndices.begin(), orderedSlotIndices.end(), [&slotOptions](std::size_t lhs, std::size_t rhs) {
        if (slotOptions[lhs].size() != slotOptions[rhs].size()) {
            return slotOptions[lhs].size() < slotOptions[rhs].size();
        }
        return lhs < rhs;
    });

    AssignmentResult best;
    best.orderedPlayerIdsByTemplate.assign(slotCount, 0);

    std::vector<PlayerId> currentAssigned(slotCount, 0);
    std::unordered_set<PlayerId> usedPlayerIds;
    usedPlayerIds.reserve(players.size());

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
        const int remainingPlayers = static_cast<int>(players.size() - usedPlayerIds.size());
        const int maxAdditionalAssignments = std::min(remainingSlots, remainingPlayers);
        if (assignedCount + maxAdditionalAssignments < best.assignedSlotCount) {
            return;
        }

        const std::size_t slotIdx = orderedSlotIndices[depth];
        const double optimisticTotalPower = totalPower + optimisticSuffix[slotIdx];
        if (assignedCount + maxAdditionalAssignments == best.assignedSlotCount && optimisticTotalPower + 1e-9 < best.totalEffectivePower) {
            return;
        }

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
                                                        const Team& team,
                                                        const std::vector<PlayerCandidate>& players,
                                                        std::size_t starterTargetCount) {
    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    AssignmentResult assignment = buildBestAssignmentForTemplate(slotTemplate, players, starterTargetCount);

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

    (void)team;
    return candidate;
}
}

TeamSheet TeamSelectionService::buildTeamSheet(const Team& team) const {
    const std::vector<PlayerCandidate> players = collectCandidates(team);
    const std::size_t starterTargetCount = std::min<std::size_t>(11, players.size());

    const FormationId preferredFormation = team.getHeadCoach().getPreferredFormation();
    const std::vector<FormationId> formationOrder = buildDeterministicFormationOrder(preferredFormation);

    FormationSelectionCandidate bestCandidate{};
    bool hasBestCandidate = false;

    for (FormationId formationId : formationOrder) {
        FormationSelectionCandidate candidate = evaluateFormationCandidate(formationId, team, players, starterTargetCount);

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
