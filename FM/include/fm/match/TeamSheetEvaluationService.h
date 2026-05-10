#pragma once

#include "fm/common/Types.h"

#include <optional>
#include <vector>

class Team;
struct TeamSheet;

struct LineupSummary {
    int assignedPlayers = 0;
    std::optional<int> averageOverall;
};

class TeamSheetEvaluationService {
public:
    static LineupSummary summarize(const TeamSheet& sheet, const Team& team);
    static LineupSummary summarizePlayerIds(const std::vector<PlayerId>& playerIds, const Team& team);
};
