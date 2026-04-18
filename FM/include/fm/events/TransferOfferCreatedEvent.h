#pragma once

#include"fm/common/Types.h"
#include"fm/common/Date.h"

struct TransferOfferCreatedEvent {
	OfferId offerId = 0;
    Date createdAt{ 1900, Month::January, 1 };
    LeagueId sellerLeagueId = 0;
    TeamId sellerTeamId = 0;
    LeagueId buyerLeagueId = 0;
    TeamId buyerTeamId = 0;
    PlayerId playerId = 0;
    Money fee = 0;
};