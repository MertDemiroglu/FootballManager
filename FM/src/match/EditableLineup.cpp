#include"fm/match/EditableLineup.h"

#include"fm/match/TeamSelectionService.h"
#include"fm/roster/Team.h"

#include<algorithm>

EditableLineup::EditableLineup(const Team& team, FormationId formationId)
    : teamId(team.getId()),
      coachId(team.getHeadCoach().getId()),
      formationId(formationId) {
    buildSlotsFromFormation(formationId);
    cacheRosterPlayerIds(team);
}

EditableLineup EditableLineup::createSeededFromAutoSelection(const Team& team,
                                                             FormationId formationId,
                                                             const TeamSelectionService& teamSelectionService) {
    EditableLineup editableLineup(team, formationId);

    TeamSheet generatedTeamSheet = teamSelectionService.buildTeamSheet(team);
    generatedTeamSheet.formation = formationId;
    editableLineup.seedFromTeamSheet(generatedTeamSheet);

    return editableLineup;
}

TeamId EditableLineup::getTeamId() const {
    return teamId;
}

CoachId EditableLineup::getCoachId() const {
    return coachId;
}

FormationId EditableLineup::getFormationId() const {
    return formationId;
}

const std::vector<EditableLineupSlot>& EditableLineup::getSlots() const {
    return slots;
}

std::optional<PlayerId> EditableLineup::getAssignedPlayerIdForSlot(std::size_t slotIndex) const {
    if (slotIndex >= slots.size()) {
        return std::nullopt;
    }

    if (slots[slotIndex].assignedPlayerId == 0) {
        return std::nullopt;
    }

    return slots[slotIndex].assignedPlayerId;
}

bool EditableLineup::isPlayerAssigned(PlayerId playerId) const {
    return findAssignedSlotIndex(playerId).has_value();
}

std::optional<std::size_t> EditableLineup::findAssignedSlotIndex(PlayerId playerId) const {
    if (playerId == 0) {
        return std::nullopt;
    }

    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (slots[i].assignedPlayerId == playerId) {
            return i;
        }
    }

    return std::nullopt;
}

std::vector<PlayerId> EditableLineup::getUnassignedPlayerIds() const {
    std::vector<PlayerId> unassigned;
    unassigned.reserve(rosterPlayerIds.size());

    for (PlayerId playerId : rosterPlayerIds) {
        if (!isPlayerAssigned(playerId)) {
            unassigned.push_back(playerId);
        }
    }

    return unassigned;
}

bool EditableLineup::isFullLineup() const {
    return std::all_of(slots.begin(), slots.end(), [](const EditableLineupSlot& slot) {
        return slot.assignedPlayerId != 0;
    });
}

bool EditableLineup::assignPlayerToSlot(PlayerId playerId, std::size_t slotIndex) {
    if (slotIndex >= slots.size() || playerId == 0 || !isPlayerInRoster(playerId)) {
        return false;
    }

    unassignPlayer(playerId);

    // Deterministic Phase 1 policy: assigning to an occupied slot replaces the
    // current occupant. The previous occupant becomes unassigned.
    slots[slotIndex].assignedPlayerId = playerId;
    return true;
}

bool EditableLineup::clearSlot(std::size_t slotIndex) {
    if (slotIndex >= slots.size()) {
        return false;
    }

    const bool hadAssignment = slots[slotIndex].assignedPlayerId != 0;
    slots[slotIndex].assignedPlayerId = 0;
    return hadAssignment;
}

bool EditableLineup::unassignPlayer(PlayerId playerId) {
    std::optional<std::size_t> slotIndex = findAssignedSlotIndex(playerId);
    if (!slotIndex.has_value()) {
        return false;
    }

    slots[*slotIndex].assignedPlayerId = 0;
    return true;
}

TeamSheet EditableLineup::exportAsTeamSheet() const {
    TeamSheet teamSheet;
    teamSheet.teamId = teamId;
    teamSheet.coachId = coachId;
    teamSheet.formation = formationId;

    for (const EditableLineupSlot& slot : slots) {
        if (slot.assignedPlayerId == 0) {
            continue;
        }

        teamSheet.startingAssignments.push_back(TeamSheetSlotAssignment{ slot.slotRole, slot.assignedPlayerId });
        teamSheet.startingPlayerIds.push_back(slot.assignedPlayerId);
    }

    return teamSheet;
}

void EditableLineup::buildSlotsFromFormation(FormationId formation) {
    slots.clear();

    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    slots.reserve(slotTemplate.size());

    for (std::size_t i = 0; i < slotTemplate.size(); ++i) {
        slots.push_back(EditableLineupSlot{ i, slotTemplate[i], 0 });
    }
}

void EditableLineup::cacheRosterPlayerIds(const Team& team) {
    rosterPlayerIds.clear();

    for (const auto& player : team.getPlayers()) {
        if (!player) {
            continue;
        }

        rosterPlayerIds.push_back(player->getId());
    }

    std::sort(rosterPlayerIds.begin(), rosterPlayerIds.end());
}

bool EditableLineup::isPlayerInRoster(PlayerId playerId) const {
    return std::binary_search(rosterPlayerIds.begin(), rosterPlayerIds.end(), playerId);
}

void EditableLineup::seedFromTeamSheet(const TeamSheet& teamSheet) {
    if (teamSheet.teamId != teamId) {
        return;
    }

    for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
        if (assignment.playerId == 0 || !isPlayerInRoster(assignment.playerId)) {
            continue;
        }

        auto targetSlotIt = std::find_if(slots.begin(), slots.end(), [&](const EditableLineupSlot& slot) {
            return slot.slotRole == assignment.slotRole && slot.assignedPlayerId == 0;
        });

        if (targetSlotIt == slots.end()) {
            continue;
        }

        assignPlayerToSlot(assignment.playerId, targetSlotIt->slotIndex);
    }
}
