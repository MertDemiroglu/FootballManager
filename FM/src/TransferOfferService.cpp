#include"TransferOfferService.h"
#include<algorithm>

#include"fm/common/DateUtils.h"
#include"fm/events/DomainEventPublisher.h"
#include"fm/competition/League.h"
#include"LeagueContext.h"
#include"fm/competition/LeagueRules.h"
#include"fm/events/PlayerTransferredEvent.h"
#include"fm/roster/Team.h"
#include"fm/events/TransferOfferCreatedEvent.h"
#include"fm/events/TransferOfferResolvedEvent.h"
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

    if (!world->getTransferRoom().isOpenForLeague(sellerLeagueId) || !world->getTransferRoom().isOpenForLeague(buyerLeagueId)) {
        return false;
    }

    return true;
}

std::optional<Date> TransferOfferService::tryGetCurrentWindowLastOpenDate(LeagueId leagueId, const Date& referenceDate) const {
    if (!world || leagueId == 0) {
        return std::nullopt;
    }

    const LeagueContext* context = world->findLeagueContext(leagueId);
    if (!context) {
        return std::nullopt;
    }

    const SeasonPlan& plan = context->getSeasonPlan();
    const TransferWindow* activeWindow = nullptr;

    if (plan.getSummerWindow().contains(referenceDate)) {
        activeWindow = &plan.getSummerWindow();
    }
    else if (plan.getWinterWindow().contains(referenceDate)) {
        activeWindow = &plan.getWinterWindow();
    }
    if (!activeWindow) {
        return std::nullopt;
    }

    Date cursor = referenceDate;
    Date lastOpenDate = referenceDate;
    while (cursor < activeWindow->endExclusive) {
        lastOpenDate = cursor;
        cursor.advanceDay();
    }

    return lastOpenDate;
}

TransferOfferResolution TransferOfferService::resolvePendingOfferInvalidity(const TransferOffer& offer, const Date& currentDate) const {
    if (!world) {
        return TransferOfferResolution::ExpiredByDeadline;
    }

    const bool sellerWindowOpen = world->getTransferRoom().isOpenForLeague(offer.sellerLeagueId);
    const bool buyerWindowOpen = world->getTransferRoom().isOpenForLeague(offer.buyerLeagueId);
    if (!sellerWindowOpen || !buyerWindowOpen) {
        return TransferOfferResolution::ExpiredByWindowClose;
    }

    if (offer.lastValidDate < currentDate) {
        return TransferOfferResolution::ExpiredByDeadline;
    }


    return TransferOfferResolution::ExpiredByDeadline;
}

OfferId TransferOfferService::createOffer(const Date& createdAt, LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId, Money fee) {
    if (!validateOfferCreation(sellerLeagueId, sellerTeamId, buyerLeagueId, buyerTeamId, playerId, fee)) {
        return 0;
    }

    const OfferId createdOfferId = nextOfferId++;
    TransferOffer offer;

    offer.id = createdOfferId;
    offer.createdAt = createdAt;

    const Date fourteenDayLimit = DateUtils::addDays(createdAt, 14);
    Date lastValidDate = fourteenDayLimit;
    TransferOfferExpiryPolicy expiryPolicy = TransferOfferExpiryPolicy::FourteenDayLimit;

    const std::optional<Date> sellerWindowLastDate = tryGetCurrentWindowLastOpenDate(sellerLeagueId, createdAt);
    const std::optional<Date> buyerWindowLastDate = tryGetCurrentWindowLastOpenDate(buyerLeagueId, createdAt);
    if (sellerWindowLastDate.has_value() && sellerWindowLastDate.value() < lastValidDate) {
        lastValidDate = sellerWindowLastDate.value();
        expiryPolicy = TransferOfferExpiryPolicy::WindowCloseLimit;
    }
    if (buyerWindowLastDate.has_value() && buyerWindowLastDate.value() < lastValidDate) {
        lastValidDate = buyerWindowLastDate.value();
        expiryPolicy = TransferOfferExpiryPolicy::WindowCloseLimit;
    }

    offer.lastValidDate = lastValidDate;
    offer.expiryPolicy = expiryPolicy;
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

bool TransferOfferService::hasPendingOfferForSellerPlayer(LeagueId sellerLeagueId, TeamId sellerTeamId, PlayerId playerId) const {
    if (sellerLeagueId == 0 || sellerTeamId == 0 || playerId == 0) {
        return false;
    }

    for (const auto& [offerId, offer] : offersById) {
        (void)offerId;
        if (offer.status != TransferOfferStatus::Pending) {
            continue;
        }
        if (offer.sellerLeagueId == sellerLeagueId &&
            offer.sellerTeamId == sellerTeamId &&
            offer.playerId == playerId) {
            return true;
        }
    }

    return false;
}

std::vector<const TransferOffer*> TransferOfferService::getPendingOffersForSellerTeam(LeagueId sellerLeagueId, TeamId sellerTeamId) const {
    std::vector<const TransferOffer*> pendingOffers;
    if (sellerLeagueId == 0 || sellerTeamId == 0) {
        return pendingOffers;
    }
    pendingOffers.reserve(offersById.size());

    for (const auto& [offerId, offer] : offersById) {
        (void)offerId;
        if (offer.status != TransferOfferStatus::Pending) {
            continue;
        }
        if (offer.sellerLeagueId != sellerLeagueId || offer.sellerTeamId != sellerTeamId) {
            continue;
        }
        pendingOffers.push_back(&offer);
    }
    std::sort(pendingOffers.begin(), pendingOffers.end(), [](const TransferOffer* lhs, const TransferOffer* rhs) {
        if (lhs == nullptr || rhs == nullptr) {
            return lhs != nullptr;
        }
        if (lhs->createdAt < rhs->createdAt) {
            return true;
        }
        if (rhs->createdAt < lhs->createdAt) {
            return false;
        }
        return lhs->id < rhs->id;
        });

    return pendingOffers;
}

int TransferOfferService::expirePendingOffers(const Date& currentDate) {
    if (!world) {
        return 0;
    }

    std::vector<OfferId> expiredOfferIds;
    expiredOfferIds.reserve(offersById.size());

    for (const auto& [offerId, offer] : offersById) {
        if (offer.status != TransferOfferStatus::Pending) {
            continue;
        }

        const bool sellerWindowOpen = world->getTransferRoom().isOpenForLeague(offer.sellerLeagueId);
        const bool buyerWindowOpen = world->getTransferRoom().isOpenForLeague(offer.buyerLeagueId);
        const bool expiredByDate = offer.lastValidDate < currentDate;
        const bool expiredByWindow = !sellerWindowOpen || !buyerWindowOpen;

        if (!expiredByDate && !expiredByWindow) {
            continue;
        }

        expiredOfferIds.push_back(offerId);
    }

    for (const OfferId offerId : expiredOfferIds) {
        auto offerIt = offersById.find(offerId);
        if (offerIt == offersById.end()) {
            continue;
        }

        TransferOffer& offer = offerIt->second;
        if (offer.status != TransferOfferStatus::Pending) {
            continue;
        }

        const TransferOfferResolution invalidityResolution = resolvePendingOfferInvalidity(offer, currentDate);
        offer.status = TransferOfferStatus::Resolved;
        world->getDomainEventPublisher().publish(TransferOfferResolvedEvent{ offer.id, invalidityResolution, offer.sellerLeagueId, offer.sellerTeamId, offer.buyerLeagueId, offer.buyerTeamId, offer.playerId, offer.fee });
    }

    return static_cast<int>(expiredOfferIds.size());
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

    const TransferOfferResolution invalidityResolution = resolvePendingOfferInvalidity(offer, transferDate);
    const bool sellerWindowOpen = world->getTransferRoom().isOpenForLeague(offer.sellerLeagueId);
    const bool buyerWindowOpen = world->getTransferRoom().isOpenForLeague(offer.buyerLeagueId);
    const bool deadlineValid = !(offer.lastValidDate < transferDate);
    if (!deadlineValid || !sellerWindowOpen || !buyerWindowOpen) {
        offer.status = TransferOfferStatus::Resolved;
        world->getDomainEventPublisher().publish(TransferOfferResolvedEvent{ offer.id, invalidityResolution, offer.sellerLeagueId, offer.sellerTeamId, offer.buyerLeagueId, offer.buyerTeamId, offer.playerId, offer.fee });
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
