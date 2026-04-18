#include"TeamSheet.h"

#include"fm/roster/Team.h"

#include<stdexcept>
#include<unordered_set>

void validateTeamSheet(const TeamSheet& teamSheet) {
    if (teamSheet.teamId == 0) {
        throw std::invalid_argument("teamsheet team id cannot be zero");
    }

    if (teamSheet.coachId == 0) {
        throw std::invalid_argument("teamsheet coach id cannot be zero");
    }

    if (!isFormationSupported(teamSheet.formation)) {
        throw std::invalid_argument("teamsheet formation is not supported");
    }

    if (teamSheet.startingAssignments.empty()) {
        if (teamSheet.startingPlayerIds.size() > 11) {
            throw std::invalid_argument("teamsheet cannot contain more than 11 starters");
        }

        std::unordered_set<PlayerId> seenIds;
        seenIds.reserve(teamSheet.startingPlayerIds.size());
        for (PlayerId playerId : teamSheet.startingPlayerIds) {
            if (playerId == 0) {
                throw std::invalid_argument("teamsheet cannot contain player id zero");
            }

            const auto [it, inserted] = seenIds.emplace(playerId);
            (void)it;
            if (!inserted) {
                throw std::invalid_argument("teamsheet cannot contain duplicate starter ids");
            }
        }

        return;
    }

    if (teamSheet.startingAssignments.size() > 11) {
        throw std::invalid_argument("teamsheet cannot contain more than 11 slot assignments");
    }

    if (teamSheet.startingPlayerIds.size() != teamSheet.startingAssignments.size()) {
        throw std::invalid_argument("teamsheet starting player ids must match slot assignment count");
    }

    std::unordered_set<PlayerId> seenIds;
    seenIds.reserve(teamSheet.startingAssignments.size());

    for (std::size_t i = 0; i < teamSheet.startingAssignments.size(); ++i) {
        const TeamSheetSlotAssignment& assignment = teamSheet.startingAssignments[i];

        if (!isSlotRoleSupported(assignment.slotRole)) {
            throw std::invalid_argument("teamsheet contains unsupported slot role");
        }

        if (assignment.playerId == 0) {
            throw std::invalid_argument("teamsheet assignment cannot contain player id zero");
        }

        const auto [it, inserted] = seenIds.emplace(assignment.playerId);
        (void)it;
        if (!inserted) {
            throw std::invalid_argument("teamsheet cannot contain duplicate assigned player ids");
        }

        if (teamSheet.startingPlayerIds[i] != assignment.playerId) {
            throw std::invalid_argument("teamsheet starting player ids must match ordered assignment ids");
        }
    }
}

void validateTeamSheetForTeam(const TeamSheet& teamSheet, const Team& team) {
    validateTeamSheet(teamSheet);

    if (teamSheet.teamId != team.getId()) {
        throw std::invalid_argument("teamsheet team id does not match target team");
    }

    if (!teamSheet.startingAssignments.empty()) {
        for (const TeamSheetSlotAssignment& assignment : teamSheet.startingAssignments) {
            if (team.findPlayerById(assignment.playerId) == nullptr) {
                throw std::invalid_argument("teamsheet contains assigned player not in team roster");
            }
        }
        return;
    }

    for (PlayerId playerId : teamSheet.startingPlayerIds) {
        if (team.findPlayerById(playerId) == nullptr) {
            throw std::invalid_argument("teamsheet contains player not in team roster");
        }
    }
}
