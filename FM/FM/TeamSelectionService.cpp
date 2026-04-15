#include "TeamSelectionService.h"

#include "Footballer.h"
#include "Formation.h"
#include "PlayerPositionBucket.h"
#include "Team.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace {
struct PlayerCandidate {
    const Footballer* player = nullptr;
    PlayerId id = 0;
    int totalPower = 0;
    FormationSlotGroup slotGroup = FormationSlotGroup::Unknown;
};

struct SelectionCandidate {
    FormationId formation = FormationId::FourFourTwo;
    std::vector<PlayerId> selectedPlayerIds;
    int strictlyMatchedSlots = 0;
    bool fullySatisfied = false;
};

bool isHigherRanked(const PlayerCandidate& lhs, const PlayerCandidate& rhs) {
    if (lhs.totalPower != rhs.totalPower) {
        return lhs.totalPower > rhs.totalPower;
    }

    return lhs.id < rhs.id;
}

std::vector<PlayerCandidate> collectSortedCandidates(const Team& team) {
    std::vector<PlayerCandidate> candidates;
    candidates.reserve(team.getPlayers().size());

    for (const auto& player : team.getPlayers()) {
        if (!player) {
            continue;
        }

        candidates.push_back(PlayerCandidate{
            player.get(),
            player->getId(),
            player->totalPower(),
            mapPlayerPositionToFormationSlotGroup(player->getPlayerPosition())
        });
    }

    std::sort(candidates.begin(), candidates.end(), isHigherRanked);
    return candidates;
}

void selectFromSlotGroup(const std::vector<PlayerCandidate>& sortedCandidates,
                         FormationSlotGroup targetGroup,
                         int requestedCount,
                         std::unordered_set<PlayerId>& selectedIds,
                         std::vector<PlayerId>& selectedPlayerIds,
                         int& strictlyMatchedSlots) {
    if (requestedCount <= 0) {
        return;
    }

    for (const PlayerCandidate& candidate : sortedCandidates) {
        if (requestedCount <= 0) {
            break;
        }

        if (candidate.slotGroup != targetGroup) {
            continue;
        }

        const auto [it, inserted] = selectedIds.emplace(candidate.id);
        (void)it;
        if (!inserted) {
            continue;
        }

        selectedPlayerIds.push_back(candidate.id);
        --requestedCount;
        ++strictlyMatchedSlots;
    }
}

SelectionCandidate buildSelectionCandidate(const std::vector<PlayerCandidate>& sortedCandidates,
                                           FormationId formation,
                                           std::size_t starterTargetCount) {
    SelectionCandidate candidate;
    candidate.formation = formation;

    std::unordered_set<PlayerId> selectedIds;
    selectedIds.reserve(starterTargetCount);

    const FormationDefinition& definition = getFormationDefinition(formation);

    selectFromSlotGroup(sortedCandidates, FormationSlotGroup::Goalkeeper, definition.goalkeeperCount,
        selectedIds, candidate.selectedPlayerIds, candidate.strictlyMatchedSlots);
    selectFromSlotGroup(sortedCandidates, FormationSlotGroup::Defender, definition.defenderCount,
        selectedIds, candidate.selectedPlayerIds, candidate.strictlyMatchedSlots);
    selectFromSlotGroup(sortedCandidates, FormationSlotGroup::Midfielder, definition.midfielderCount,
        selectedIds, candidate.selectedPlayerIds, candidate.strictlyMatchedSlots);
    selectFromSlotGroup(sortedCandidates, FormationSlotGroup::Forward, definition.forwardCount,
        selectedIds, candidate.selectedPlayerIds, candidate.strictlyMatchedSlots);

    for (const PlayerCandidate& fallbackCandidate : sortedCandidates) {
        if (candidate.selectedPlayerIds.size() >= starterTargetCount) {
            break;
        }

        const auto [it, inserted] = selectedIds.emplace(fallbackCandidate.id);
        (void)it;
        if (!inserted) {
            continue;
        }

        candidate.selectedPlayerIds.push_back(fallbackCandidate.id);
    }

    const std::size_t strictTargetCount = std::min<std::size_t>(11, starterTargetCount);
    candidate.fullySatisfied = (candidate.strictlyMatchedSlots == static_cast<int>(strictTargetCount));

    return candidate;
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
}

TeamSheet TeamSelectionService::buildTeamSheet(const Team& team) const {
    const std::vector<PlayerCandidate> sortedCandidates = collectSortedCandidates(team);
    const std::size_t starterTargetCount = std::min<std::size_t>(11, sortedCandidates.size());

    const FormationId preferredFormation = team.getHeadCoach().getPreferredFormation();
    const std::vector<FormationId> formationOrder = buildDeterministicFormationOrder(preferredFormation);

    SelectionCandidate bestPartialCandidate{};
    bool hasBestPartial = false;

    for (FormationId formationId : formationOrder) {
        SelectionCandidate candidate = buildSelectionCandidate(sortedCandidates, formationId, starterTargetCount);

        if (candidate.fullySatisfied) {
            TeamSheet teamSheet{ team.getId(), team.getHeadCoach().getId(), candidate.formation, std::move(candidate.selectedPlayerIds) };
            validateTeamSheetForTeam(teamSheet, team);
            return teamSheet;
        }

        if (!hasBestPartial || candidate.strictlyMatchedSlots > bestPartialCandidate.strictlyMatchedSlots) {
            bestPartialCandidate = std::move(candidate);
            hasBestPartial = true;
        }
    }

    if (!hasBestPartial) {
        bestPartialCandidate.formation = isFormationSupported(preferredFormation)
            ? preferredFormation
            : getSupportedFormationIds().front();
    }

    TeamSheet teamSheet{ team.getId(), team.getHeadCoach().getId(), bestPartialCandidate.formation, std::move(bestPartialCandidate.selectedPlayerIds) };
    validateTeamSheetForTeam(teamSheet, team);
    return teamSheet;
}
