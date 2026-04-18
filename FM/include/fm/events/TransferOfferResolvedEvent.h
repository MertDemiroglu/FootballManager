#pragma once

#include"TransferOffer.h"
#include"fm/common/Types.h"

struct TransferOfferResolvedEvent {
    OfferId offerId = 0;
    TransferOfferResolution resolution = TransferOfferResolution::ExpiredByDeadline;
    LeagueId sellerLeagueId = 0;
    TeamId sellerTeamId = 0;
    LeagueId buyerLeagueId = 0;
    TeamId buyerTeamId = 0;
    PlayerId playerId = 0;
    Money fee = 0;
};