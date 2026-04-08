#pragma once

#include"Date.h"
#include"Types.h"

struct PlayerTransferredEvent {
    Date date{ 1900, Month::January, 1 };
    OfferId offerId = 0;
    LeagueId sellerLeagueId = 0;
    TeamId sellerTeamId = 0;
    LeagueId buyerLeagueId = 0;
    TeamId buyerTeamId = 0;
    PlayerId playerId = 0;
    Money fee = 0;
};