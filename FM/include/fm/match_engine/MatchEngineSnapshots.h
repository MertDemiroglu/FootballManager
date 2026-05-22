#pragma once

#include"fm/common/Types.h"
#include"fm/match/PlayerConditionState.h"
#include"fm/match/TacticalSetup.h"
#include"fm/match/TeamSheet.h"
#include"fm/roster/PlayerAttributes.h"
#include"fm/roster/PlayerPosition.h"

#include<string>
#include<vector>

struct MatchPlayerSnapshot {
    PlayerId playerId = 0;
    TeamId teamId = 0;
    PlayerPosition position = PlayerPosition::Unknown;
    PlayerAttributes attributes;
    PlayerConditionState conditionState;
    int baseOverall = 0;
    int totalPower = 0;
};

struct MatchTeamSnapshot {
    LeagueId leagueId = 0;
    TeamId teamId = 0;
    std::string teamName;
    TacticalSetup tacticalSetup;
    TeamSheet teamSheet;
    std::vector<MatchPlayerSnapshot> players;
};
