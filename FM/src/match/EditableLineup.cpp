#include"fm/match/EditableLineup.h"

#include"fm/match/TeamSelectionService.h"
#include"fm/roster/Team.h"

#include<algorithm>
#include<unordered_set>

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

    TeamSheet generatedTeamSheet = teamSelectionService.buildTeamSheet(team, formationId);
    editableLineup.applyTeamSheet(generatedTeamSheet);

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

TacticalSetup EditableLineup::getTacticalSetup() const {
    return tacticalSetup;
}

const std::vector<EditableLineupSlot>& EditableLineup::getSlots() const {
    return lineupSlots;
}

const std::vector<PlayerId>& EditableLineup::getSubstitutePlayerIds() const {
    return substitutePlayerIds;
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

    // Assignment is also the explicit move path: remove the player from any
    // old slot before writing the target slot. If occupied, the target's
    // previous player is replaced and becomes unassigned.
    unassignPlayer(playerId);
    removeSubstitutePlayer(playerId);
    lineupSlots[slotIndex].assignedPlayerId = playerId;
    return true;
}

bool EditableLineup::changeFormation(FormationId newFormationId) {
    if (newFormationId == formationId || !isFormationSupported(newFormationId)) {
        return false;
    }

    const std::vector<FormationSlotRole>* slotTemplate = nullptr;
    try {
        slotTemplate = &getFormationSlotTemplate(newFormationId);
    }
    catch (...) {
        return false;
    }

    if (!slotTemplate || slotTemplate->empty()) {
        return false;
    }

    formationId = newFormationId;
    buildSlotsFromFormation(newFormationId);
    return true;
}

bool EditableLineup::swapSlots(std::size_t firstSlotIndex, std::size_t secondSlotIndex) {
    if (firstSlotIndex >= lineupSlots.size() || secondSlotIndex >= lineupSlots.size()) {
        return false;
    }

    if (firstSlotIndex == secondSlotIndex) {
        return false;
    }

    const PlayerId firstPlayerId = lineupSlots[firstSlotIndex].assignedPlayerId;
    const PlayerId secondPlayerId = lineupSlots[secondSlotIndex].assignedPlayerId;
    if (firstPlayerId == 0 && secondPlayerId == 0) {
        return false;
    }

    lineupSlots[firstSlotIndex].assignedPlayerId = secondPlayerId;
    lineupSlots[secondSlotIndex].assignedPlayerId = firstPlayerId;
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

void EditableLineup::setSubstitutePlayerIds(std::vector<PlayerId> playerIds) {
    substitutePlayerIds.clear();
    substitutePlayerIds.reserve(std::min<std::size_t>(playerIds.size(), kMaxSubstituteCount));

    for (PlayerId playerId : playerIds) {
        if (substitutePlayerIds.size() >= kMaxSubstituteCount) {
            break;
        }
        if (playerId == 0 || !isPlayerInRoster(playerId) || isPlayerAssigned(playerId)) {
            continue;
        }
        if (std::find(substitutePlayerIds.begin(), substitutePlayerIds.end(), playerId) != substitutePlayerIds.end()) {
            continue;
        }

        substitutePlayerIds.push_back(playerId);
    }
}

void EditableLineup::setTacticalSetup(TacticalSetup newTacticalSetup) {
    tacticalSetup = newTacticalSetup;
}

void EditableLineup::applyTeamSheet(const TeamSheet& teamSheet) {
    for (EditableLineupSlot& slot : lineupSlots) {
        slot.assignedPlayerId = 0;
    }

    if (teamSheet.teamId != teamId || teamSheet.formation != formationId) {
        return;
    }

    std::vector<PlayerId> appliedPlayerIds;
    appliedPlayerIds.reserve(teamSheet.startingAssignments.size());

    for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
        if (assignment.playerId == 0 || !isPlayerInRoster(assignment.playerId)) {
            continue;
        }

        const bool alreadyApplied = std::find(appliedPlayerIds.begin(), appliedPlayerIds.end(), assignment.playerId)
            != appliedPlayerIds.end();
        if (alreadyApplied) {
            continue;
        }

        auto targetSlotIt = std::find_if(lineupSlots.begin(), lineupSlots.end(), [&](const EditableLineupSlot& slot) {
            return slot.slotRole == assignment.slotRole && slot.assignedPlayerId == 0;
        });

        if (targetSlotIt == lineupSlots.end()) {
            continue;
        }

        targetSlotIt->assignedPlayerId = assignment.playerId;
        appliedPlayerIds.push_back(assignment.playerId);
    }

    tacticalSetup = teamSheet.tacticalSetup;
    setSubstitutePlayerIds(teamSheet.substitutePlayerIds);
}

TeamSheet EditableLineup::exportAsTeamSheet() const {
    TeamSheet teamSheet;
    teamSheet.teamId = teamId;
    teamSheet.coachId = coachId;
    teamSheet.formation = formationId;
    teamSheet.tacticalSetup = tacticalSetup;

    for (const EditableLineupSlot& slot : lineupSlots) {
        if (slot.assignedPlayerId == 0) {
            continue;
        }

        teamSheet.startingAssignments.push_back(TeamSheetSlotAssignment{ slot.slotRole, slot.assignedPlayerId });
        teamSheet.startingPlayerIds.push_back(slot.assignedPlayerId);
    }

    teamSheet.substitutePlayerIds = substitutePlayerIds;
    return teamSheet;
}

std::optional<TeamSheet> EditableLineup::toTeamSheetIfComplete() const {
    if (teamId == 0 || !isFormationSupported(formationId) || lineupSlots.empty()) {
        return std::nullopt;
    }

    std::unordered_set<PlayerId> assignedPlayerIds;
    assignedPlayerIds.reserve(lineupSlots.size());

    TeamSheet teamSheet;
    teamSheet.teamId = teamId;
    teamSheet.coachId = coachId;
    teamSheet.formation = formationId;
    teamSheet.tacticalSetup = tacticalSetup;
    teamSheet.startingAssignments.reserve(lineupSlots.size());
    teamSheet.startingPlayerIds.reserve(lineupSlots.size());

    for (const EditableLineupSlot& slot : lineupSlots) {
        if (slot.assignedPlayerId == 0 || slot.slotRole == FormationSlotRole::Unknown) {
            return std::nullopt;
        }
        if (!isPlayerInRoster(slot.assignedPlayerId)) {
            return std::nullopt;
        }

        const auto [_, inserted] = assignedPlayerIds.insert(slot.assignedPlayerId);
        if (!inserted) {
            return std::nullopt;
        }

        teamSheet.startingAssignments.push_back(TeamSheetSlotAssignment{ slot.slotRole, slot.assignedPlayerId });
        teamSheet.startingPlayerIds.push_back(slot.assignedPlayerId);
    }

    teamSheet.substitutePlayerIds.reserve(substitutePlayerIds.size());
    for (PlayerId playerId : substitutePlayerIds) {
        if (playerId == 0 || !isPlayerInRoster(playerId) || assignedPlayerIds.find(playerId) != assignedPlayerIds.end()) {
            return std::nullopt;
        }
        if (std::find(teamSheet.substitutePlayerIds.begin(), teamSheet.substitutePlayerIds.end(), playerId)
            != teamSheet.substitutePlayerIds.end()) {
            return std::nullopt;
        }

        teamSheet.substitutePlayerIds.push_back(playerId);
    }

    if (teamSheet.substitutePlayerIds.size() > kMaxSubstituteCount) {
        return std::nullopt;
    }

    if (teamSheet.startingAssignments.size() != lineupSlots.size()
        || teamSheet.startingPlayerIds.size() != lineupSlots.size()) {
        return std::nullopt;
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

void EditableLineup::removeSubstitutePlayer(PlayerId playerId) {
    substitutePlayerIds.erase(
        std::remove(substitutePlayerIds.begin(), substitutePlayerIds.end(), playerId),
        substitutePlayerIds.end());
}
