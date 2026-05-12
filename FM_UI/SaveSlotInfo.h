#pragma once

#include<QString>

// Save slot info describes a full-world save DB across all leagues.
// managedLeagueId/managedTeamId only identify the user's managed club.
struct SaveSlotInfo {
    QString saveSlotId;
    QString saveName;
    QString managerName;
    int managedLeagueId = 0;
    int managedTeamId = 0;
    QString managedTeamName;
    QString managedLeagueName;
    QString currentDate;
    QString createdAtUtc;
    QString updatedAtUtc;
    int schemaVersion = 1;
    int worldVersion = 1;
    bool isValid = false;
};
