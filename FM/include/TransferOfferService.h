#pragma once

#include<unordered_map>
#include<vector>

#include"fm/common/Date.h"
#include"TransferOffer.h"
#include"fm/common/Types.h"
#include<optional>

class World;

class TransferOfferService {
private:
	World* world = nullptr;
	std::unordered_map<OfferId, TransferOffer> offersById;
	OfferId nextOfferId = 1;

	bool hasValidOfferIdentifiers(LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId) const;
	bool hasDuplicatePendingOffer(LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId) const;
	bool validateOfferCreation(LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId, Money fee) const;

	std::optional<Date> tryGetCurrentWindowLastOpenDate(LeagueId leagueId, const Date& referenceDate) const;
	TransferOfferResolution resolvePendingOfferInvalidity(const TransferOffer& offer, const Date& currentDate) const;

public:
	TransferOfferService();
	explicit TransferOfferService(World& world);

	void bindWorld(World& world);

	OfferId createOffer(const Date& createdAt, LeagueId sellerLeagueId, TeamId sellerTeamId, LeagueId buyerLeagueId, TeamId buyerTeamId, PlayerId playerId, Money fee);
	bool hasPendingOfferForSellerPlayer(LeagueId sellerLeagueId, TeamId sellerTeamId, PlayerId playerId) const;
	std::vector<const TransferOffer*> getPendingOffersForSellerTeam(LeagueId sellerLeagueId, TeamId sellerTeamId) const;
	int expirePendingOffers(const Date& currentDate);

	bool acceptOffer(OfferId offerId);
	bool acceptOffer(OfferId offerId, const Date& transferDate);
	bool rejectOffer(OfferId offerId);

	const TransferOffer* findOfferById(OfferId offerId) const;
};
