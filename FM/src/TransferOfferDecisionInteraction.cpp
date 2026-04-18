#include "TransferOfferDecisionInteraction.h"

TransferOfferDecisionInteraction::TransferOfferDecisionInteraction(OfferId offerId,
    LeagueId sellerLeagueId,
    TeamId sellerTeamId,
    LeagueId buyerLeagueId,
    TeamId buyerTeamId,
    PlayerId playerId,
    Money fee)
    : GameInteraction(Kind::TransferOfferDecision, true),
    offerId(offerId),
    sellerLeagueId(sellerLeagueId),
    sellerTeamId(sellerTeamId),
    buyerLeagueId(buyerLeagueId),
    buyerTeamId(buyerTeamId),
    playerId(playerId),
    fee(fee) {
}

OfferId TransferOfferDecisionInteraction::getOfferId() const {
    return offerId;
}

LeagueId TransferOfferDecisionInteraction::getSellerLeagueId() const {
    return sellerLeagueId;
}

TeamId TransferOfferDecisionInteraction::getSellerTeamId() const {
    return sellerTeamId;
}

LeagueId TransferOfferDecisionInteraction::getBuyerLeagueId() const {
    return buyerLeagueId;
}

TeamId TransferOfferDecisionInteraction::getBuyerTeamId() const {
    return buyerTeamId;
}

PlayerId TransferOfferDecisionInteraction::getPlayerId() const {
    return playerId;
}

Money TransferOfferDecisionInteraction::getFee() const {
    return fee;
}