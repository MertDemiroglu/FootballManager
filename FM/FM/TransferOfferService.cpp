#include"TransferOfferService.h"

#include"DomainEventPublisher.h"
#include"League.h"
#include"PlayerTransferredEvent.h"
#include"Team.h"
#include"TransferOfferCreatedEvent.h"
#include"TransferOfferResolvedEvent.h"
#include"TransferRoom.h"
#include"World.h"

TransferOfferService::TransferOfferService() = default;

TransferOfferService::TransferOfferService(World& worldRef)
    : world(&worldRef) {
}

void TransferOfferService::bindWorld(World& worldRef) {
    world = &worldRef;
}

bool TransferOfferService::hasValidOfferIdentifiers(LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId) const {
    return sellerLeagueId != 0 && sellerTeamId != 0 && buyerLeagueId != 0 && buyerTeamId != 0 && playerId != 0;
}

bool TransferOfferService::hasDuplicatePendingOffer(LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId) const {
    for (const auto& [offerId, offer] : offersById) {
        (void)offerId;
        if (offer.status != TransferOfferStatus::Pending) {
            continue;
        }
        if (offer.sellerLeagueId == sellerLeagueId &&
            offer.sellerTeamId == sellerTeamId &&
            offer.buyerLeagueId == buyerLeagueId &&
            offer.buyerTeamId == buyerTeamId &&
            offer.playerId == playerId) {
            return true;
        }
    }
    return false;
}

bool TransferOfferService::validateOfferCreation(LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId, Money fee) const {
    if (!world) {
        return false;
    }

    if (!hasValidOfferIdentifiers(sellerLeagueId, sellerTeamId, buyerLeagueId, buyerTeamId, playerId)) {
        return false;
    }

    if (fee <= 0) {
        return false;
    }

    if (sellerLeagueId == buyerLeagueId && sellerTeamId == buyerTeamId) {
        return false;
    }

    const League* sellerLeague = world->findLeagueById(sellerLeagueId);
    const League* buyerLeague = world->findLeagueById(buyerLeagueId);
    if (!sellerLeague || !buyerLeague) {
        return false;
    }

    const Team* sellerTeam = sellerLeague->findTeamById(sellerTeamId);
    const Team* buyerTeam = buyerLeague->findTeamById(buyerTeamId);
    if (!sellerTeam || !buyerTeam) {
        return false;
    }

    if (!sellerTeam->findPlayerById(playerId)) {
        return false;
    }

    if (hasDuplicatePendingOffer(sellerLeagueId, sellerTeamId, buyerLeagueId, buyerTeamId, playerId)) {
        return false;
    }

    return true;
}

OfferId TransferOfferService::createOffer(const Date& createdAt, LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId, Money fee) {
    if (!validateOfferCreation(sellerLeagueId, sellerTeamId, buyerLeagueId, buyerTeamId, playerId, fee)) {
        return 0;
    }

    const OfferId createdOfferId = nextOfferId++;
    TransferOffer offer;
    offer.id = createdOfferId;
    offer.createdAt = createdAt;
    offer.sellerLeagueId = sellerLeagueId;
    offer.sellerTeamId = sellerTeamId;
    offer.buyerLeagueId = buyerLeagueId;
    offer.buyerTeamId = buyerTeamId;
    offer.playerId = playerId;
    offer.fee = fee;
    offer.status = TransferOfferStatus::Pending;
    offersById.emplace(createdOfferId, offer);

    world->getDomainEventPublisher().publish(TransferOfferCreatedEvent{
        createdOfferId,
        createdAt,
        sellerLeagueId,
        sellerTeamId,
        buyerLeagueId,
        buyerTeamId,
        playerId,
        fee });

    return createdOfferId;
}

bool TransferOfferService::acceptOffer(OfferId offerId) {
    const TransferOffer* offer = findOfferById(offerId);
    if (!offer) {
        return false;
    }
    return acceptOffer(offerId, offer->createdAt);
}

bool TransferOfferService::acceptOffer(OfferId offerId, const Date& transferDate) {
    if (!world || offerId == 0) {
        return false;
    }

    const auto offerIt = offersById.find(offerId);
    if (offerIt == offersById.end()) {
        return false;
    }

    TransferOffer& offer = offerIt->second;
    if (offer.status != TransferOfferStatus::Pending) {
        return false;
    }

    const bool transferred = world->getTransferRoom().transferPlayer(
        offer.sellerLeagueId,
        offer.sellerTeamId,
        offer.buyerLeagueId,
        offer.buyerTeamId,
        offer.playerId,
        offer.fee);

    if (!transferred) {
        return false;
    }

    offer.status = TransferOfferStatus::Resolved;

    world->getDomainEventPublisher().publish(TransferOfferResolvedEvent{
        offer.id,
        TransferOfferResolution::Accepted,
        offer.sellerLeagueId,
        offer.sellerTeamId,
        offer.buyerLeagueId,
        offer.buyerTeamId,
        offer.playerId,
        offer.fee });

    world->getDomainEventPublisher().publish(PlayerTransferredEvent{
        transferDate,
        offer.id,
        offer.sellerLeagueId,
        offer.sellerTeamId,
        offer.buyerLeagueId,
        offer.buyerTeamId,
        offer.playerId,
        offer.fee });

    return true;
}

bool TransferOfferService::rejectOffer(OfferId offerId) {
    if (!world || offerId == 0) {
        return false;
    }

    const auto offerIt = offersById.find(offerId);
    if (offerIt == offersById.end()) {
        return false;
    }

    TransferOffer& offer = offerIt->second;
    if (offer.status != TransferOfferStatus::Pending) {
        return false;
    }

    offer.status = TransferOfferStatus::Resolved;

    world->getDomainEventPublisher().publish(TransferOfferResolvedEvent{
        offer.id,
        TransferOfferResolution::Rejected,
        offer.sellerLeagueId,
        offer.sellerTeamId,
        offer.buyerLeagueId,
        offer.buyerTeamId,
        offer.playerId,
        offer.fee });

    return true;
}

const TransferOffer* TransferOfferService::findOfferById(OfferId offerId) const {
    const auto it = offersById.find(offerId);
    if (it == offersById.end()) {
        return nullptr;
    }
    return &it->second;
}