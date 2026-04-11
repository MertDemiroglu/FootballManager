#pragma once

#include"Types.h"
#include"Date.h"

enum class TransferOfferStatus {
	Pending,
	Resolved
};

enum class TransferOfferResolution {
	Accepted,
	Rejected,
    Expired
};

struct TransferOffer {
	OfferId id = 0;
	Date createdAt{ 1900, Month::January, 1 };
	Date expiresAt{ 1900, Month::January, 1 };
	LeagueId sellerLeagueId = 0;
	TeamId sellerTeamId = 0;
	LeagueId buyerLeagueId = 0;
	TeamId buyerTeamId = 0;
	PlayerId playerId = 0;
	Money fee = 0;
	TransferOfferStatus status = TransferOfferStatus::Pending;
};