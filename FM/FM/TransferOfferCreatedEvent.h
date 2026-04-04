#pragma once

#include"Types.h"

struct TransferOfferCreatedEvent {
	OfferId offerId = 0;
    LeagueId sellerLeagueId = 0;
    TeamId sellerTeamId = 0;
    LeagueId buyerLeagueId = 0;
    TeamId buyerTeamId = 0;
    PlayerId playerId = 0;
    Money fee = 0;
};