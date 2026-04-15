#include "TeamSheet.h"

#include "Team.h"

#include <stdexcept>
#include <unordered_set>

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
}
