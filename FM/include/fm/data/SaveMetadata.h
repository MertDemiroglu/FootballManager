#pragma once

#include "fm/common/Types.h"

#include <string>

// This metadata describes the save slot. The save DB contains the full world
// state across all leagues; managedLeagueId/managedTeamId only identify the
// user's managed club.
struct SaveMetadata {
    std::string saveSlotId;
    std::string saveName;
    std::string managerName;
    LeagueId managedLeagueId = 0;
    TeamId managedTeamId = 0;
    std::string currentDate;
    std::string createdAtUtc;
    std::string updatedAtUtc;
    int schemaVersion = 1;
    int worldVersion = 1;
};
