#pragma once

#include"fm/common/Types.h"
#include"fm/match/TeamSheet.h"
#include"fm/roster/Formation.h"

#include<cstddef>
#include<optional>
#include<vector>

class Team;
class TeamSelectionService;

struct EditableLineupSlot {
    std::size_t slotIndex = 0;
    FormationSlotRole slotRole = FormationSlotRole::Unknown;
    PlayerId assignedPlayerId = 0;
};

class EditableLineup {
public:
    EditableLineup(const Team& team, FormationId formationId);

    static EditableLineup createSeededFromAutoSelection(const Team& team,
                                                        FormationId formationId,
                                                        const TeamSelectionService& teamSelectionService);

    TeamId getTeamId() const;
    CoachId getCoachId() const;
    FormationId getFormationId() const;
    const std::vector<EditableLineupSlot>& getSlots() const;

    std::optional<PlayerId> getAssignedPlayerIdForSlot(std::size_t slotIndex) const;
    bool isPlayerAssigned(PlayerId playerId) const;
    std::optional<std::size_t> findAssignedSlotIndex(PlayerId playerId) const;
    std::vector<PlayerId> getUnassignedPlayerIds() const;
    bool isFullLineup() const;

    bool assignPlayerToSlot(PlayerId playerId, std::size_t slotIndex);
    bool changeFormation(FormationId newFormationId);
    bool swapSlots(std::size_t firstSlotIndex, std::size_t secondSlotIndex);
    bool clearSlot(std::size_t slotIndex);
    bool unassignPlayer(PlayerId playerId);

    TeamSheet exportAsTeamSheet() const;

private:
    TeamId teamId = 0;
    CoachId coachId = 0;
    FormationId formationId = FormationId::FourFourTwo;
    std::vector<EditableLineupSlot> lineupSlots;
    std::vector<PlayerId> rosterPlayerIds;

    void buildSlotsFromFormation(FormationId formationId);
    void cacheRosterPlayerIds(const Team& team);
    bool isPlayerInRoster(PlayerId playerId) const;
    void seedFromTeamSheet(const TeamSheet& teamSheet);
};
