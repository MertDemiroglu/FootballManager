#include"fm/match/TeamSheet.h"

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

    std::unordered_set<PlayerId> starterIds;

    if (teamSheet.startingAssignments.empty()) {
        if (teamSheet.startingPlayerIds.size() > 11) {
            throw std::invalid_argument("teamsheet cannot contain more than 11 starters");
        }

        starterIds.reserve(teamSheet.startingPlayerIds.size());
        for (PlayerId playerId : teamSheet.startingPlayerIds) {
            if (playerId == 0) {
                throw std::invalid_argument("teamsheet cannot contain player id zero");
            }

            const auto [it, inserted] = starterIds.emplace(playerId);
            (void)it;
            if (!inserted) {
                throw std::invalid_argument("teamsheet cannot contain duplicate starter ids");
            }
        }
    } else {
        if (teamSheet.startingAssignments.size() > 11) {
            throw std::invalid_argument("teamsheet cannot contain more than 11 slot assignments");
        }

        if (teamSheet.startingPlayerIds.size() != teamSheet.startingAssignments.size()) {
            throw std::invalid_argument("teamsheet starting player ids must match slot assignment count");
        }

        starterIds.reserve(teamSheet.startingAssignments.size());
        std::unordered_set<std::size_t> seenSlotIndices;
        seenSlotIndices.reserve(teamSheet.startingAssignments.size());
        const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(teamSheet.formation);

        for (std::size_t i = 0; i < teamSheet.startingAssignments.size(); ++i) {
            const TeamSheetSlotAssignment& assignment = teamSheet.startingAssignments[i];

            if (assignment.slotIndex >= slotTemplate.size()) {
                throw std::invalid_argument("teamsheet assignment contains invalid slot index");
            }
            if (slotTemplate[assignment.slotIndex] != assignment.slotRole) {
                throw std::invalid_argument("teamsheet assignment slot role does not match formation template");
            }
            if (!seenSlotIndices.insert(assignment.slotIndex).second) {
                throw std::invalid_argument("teamsheet cannot contain duplicate slot assignments");
            }

            if (!isSlotRoleSupported(assignment.slotRole)) {
                throw std::invalid_argument("teamsheet contains unsupported slot role");
            }

            if (assignment.playerId == 0) {
                throw std::invalid_argument("teamsheet assignment cannot contain player id zero");
            }

            const auto [it, inserted] = starterIds.emplace(assignment.playerId);
            (void)it;
            if (!inserted) {
                throw std::invalid_argument("teamsheet cannot contain duplicate assigned player ids");
            }

            if (teamSheet.startingPlayerIds[i] != assignment.playerId) {
                throw std::invalid_argument("teamsheet starting player ids must match ordered assignment ids");
            }
        }
    }

    if (teamSheet.substitutePlayerIds.size() > kMaxSubstituteCount) {
        throw std::invalid_argument("teamsheet cannot contain more than 10 substitutes");
    }

    std::unordered_set<PlayerId> substituteIds;
    substituteIds.reserve(teamSheet.substitutePlayerIds.size());
    for (PlayerId playerId : teamSheet.substitutePlayerIds) {
        if (playerId == 0) {
            throw std::invalid_argument("teamsheet substitute cannot contain player id zero");
        }

        if (starterIds.find(playerId) != starterIds.end()) {
            throw std::invalid_argument("teamsheet starters and substitutes cannot overlap");
        }

        const auto [it, inserted] = substituteIds.emplace(playerId);
        (void)it;
        if (!inserted) {
            throw std::invalid_argument("teamsheet cannot contain duplicate substitute ids");
        }
    }
}

void validateTeamSheetForTeam(const TeamSheet& teamSheet, const Team& team) {
    validateTeamSheet(teamSheet);

    if (teamSheet.teamId != team.getId()) {
        throw std::invalid_argument("teamsheet team id does not match target team");
    }

    for (PlayerId playerId : teamSheet.startingPlayerIds) {
        if (team.findPlayerById(playerId) == nullptr) {
            throw std::invalid_argument("teamsheet contains player not in team roster");
        }
    }

    for (PlayerId playerId : teamSheet.substitutePlayerIds) {
        if (team.findPlayerById(playerId) == nullptr) {
            throw std::invalid_argument("teamsheet contains substitute player not in team roster");
        }
    }
}
