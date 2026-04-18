#pragma once

#include"GameInteraction.h"
#include"fm/common/Types.h"

class TransferOfferDecisionInteraction : public GameInteraction {
private:
    OfferId offerId;
    LeagueId sellerLeagueId;
    TeamId sellerTeamId;
    LeagueId buyerLeagueId;
    TeamId buyerTeamId;
    PlayerId playerId;
    Money fee;

public:
    TransferOfferDecisionInteraction(OfferId offerId, LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId, Money fee);

    OfferId getOfferId() const;
    LeagueId getSellerLeagueId() const;
    TeamId getSellerTeamId() const;
    LeagueId getBuyerLeagueId() const;
    TeamId getBuyerTeamId() const;
    PlayerId getPlayerId() const;
    Money getFee() const;
};