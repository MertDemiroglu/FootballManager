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
    return lineupSlots;
}

std::optional<PlayerId> EditableLineup::getAssignedPlayerIdForSlot(std::size_t slotIndex) const {
    if (slotIndex >= lineupSlots.size()) {
        return std::nullopt;
    }

    if (lineupSlots[slotIndex].assignedPlayerId == 0) {
        return std::nullopt;
    }

    return lineupSlots[slotIndex].assignedPlayerId;
}

bool EditableLineup::isPlayerAssigned(PlayerId playerId) const {
    return findAssignedSlotIndex(playerId).has_value();
}

std::optional<std::size_t> EditableLineup::findAssignedSlotIndex(PlayerId playerId) const {
    if (playerId == 0) {
        return std::nullopt;
    }

    for (std::size_t i = 0; i < lineupSlots.size(); ++i) {
        if (lineupSlots[i].assignedPlayerId == playerId) {
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
    return std::all_of(lineupSlots.begin(), lineupSlots.end(), [](const EditableLineupSlot& slot) {
        return slot.assignedPlayerId != 0;
    });
}

bool EditableLineup::assignPlayerToSlot(PlayerId playerId, std::size_t slotIndex) {
    if (slotIndex >= lineupSlots.size() || playerId == 0 || !isPlayerInRoster(playerId)) {
        return false;
    }

    unassignPlayer(playerId);

    // Deterministic Phase 1 policy: assigning to an occupied slot replaces the
    // current occupant. The previous occupant becomes unassigned.
    lineupSlots[slotIndex].assignedPlayerId = playerId;
    return true;
}

bool EditableLineup::clearSlot(std::size_t slotIndex) {
    if (slotIndex >= lineupSlots.size()) {
        return false;
    }

    const bool hadAssignment = lineupSlots[slotIndex].assignedPlayerId != 0;
    lineupSlots[slotIndex].assignedPlayerId = 0;
    return hadAssignment;
}

bool EditableLineup::unassignPlayer(PlayerId playerId) {
    std::optional<std::size_t> slotIndex = findAssignedSlotIndex(playerId);
    if (!slotIndex.has_value()) {
        return false;
    }

    lineupSlots[*slotIndex].assignedPlayerId = 0;
    return true;
}

TeamSheet EditableLineup::exportAsTeamSheet() const {
    TeamSheet teamSheet;
    teamSheet.teamId = teamId;
    teamSheet.coachId = coachId;
    teamSheet.formation = formationId;

    for (const EditableLineupSlot& slot : lineupSlots) {
        if (slot.assignedPlayerId == 0) {
            continue;
        }

        teamSheet.startingAssignments.push_back(TeamSheetSlotAssignment{ slot.slotRole, slot.assignedPlayerId });
        teamSheet.startingPlayerIds.push_back(slot.assignedPlayerId);
    }

    return teamSheet;
}

void EditableLineup::buildSlotsFromFormation(FormationId formation) {
    lineupSlots.clear();

    const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formation);
    lineupSlots.reserve(slotTemplate.size());

    for (std::size_t i = 0; i < slotTemplate.size(); ++i) {
        lineupSlots.push_back(EditableLineupSlot{ i, slotTemplate[i], 0 });
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

        auto targetSlotIt = std::find_if(lineupSlots.begin(), lineupSlots.end(), [&](const EditableLineupSlot& slot) {
            return slot.slotRole == assignment.slotRole && slot.assignedPlayerId == 0;
        });

        if (targetSlotIt == lineupSlots.end()) {
            continue;
        }

        assignPlayerToSlot(assignment.playerId, targetSlotIt->slotIndex);
    }
}
