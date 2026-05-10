#include "fm/match/TeamSheetEvaluationService.h"

#include "fm/match/TeamSheet.h"
#include "fm/roster/Footballer.h"
#include "fm/roster/Team.h"

#include <cmath>

LineupSummary TeamSheetEvaluationService::summarize(const TeamSheet& sheet, const Team& team) {
    std::vector<PlayerId> playerIds;
    playerIds.reserve(sheet.startingAssignments.size());
    for (const TeamSheetSlotAssignment& assignment : sheet.startingAssignments) {
        playerIds.push_back(assignment.playerId);
    }
    return summarizePlayerIds(playerIds, team);
}

LineupSummary TeamSheetEvaluationService::summarizePlayerIds(const std::vector<PlayerId>& playerIds, const Team& team) {
    LineupSummary summary;
    int totalOverall = 0;
    int overallCount = 0;

    for (const PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }

        ++summary.assignedPlayers;
        const Footballer* player = team.findPlayerById(playerId);
        if (!player) {
            continue;
        }

        const int overall = player->totalPower();
        if (overall <= 0) {
            continue;
        }

        totalOverall += overall;
        ++overallCount;
    }

    if (overallCount > 0) {
        summary.averageOverall = static_cast<int>(std::lround(static_cast<double>(totalOverall) / overallCount));
    }

    return summary;
}
