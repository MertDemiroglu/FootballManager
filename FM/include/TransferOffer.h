#pragma once

#include"fm/common/Types.h"
#include"fm/common/Date.h"

enum class TransferOfferStatus {
	Pending,
	Resolved
};

enum class TransferOfferResolution {
	Accepted,
	Rejected,
	ExpiredByDeadline,
	ExpiredByWindowClose
};

enum class TransferOfferExpiryPolicy {
	FourteenDayLimit,
	WindowCloseLimit
};

struct TransferOffer {
	OfferId id = 0;
	Date createdAt{ 1900, Month::January, 1 };
	Date lastValidDate{ 1900, Month::January, 1 };
	TransferOfferExpiryPolicy expiryPolicy = TransferOfferExpiryPolicy::FourteenDayLimit;
	LeagueId sellerLeagueId = 0;
	TeamId sellerTeamId = 0;
	LeagueId buyerLeagueId = 0;
	TeamId buyerTeamId = 0;
	PlayerId playerId = 0;
	Money fee = 0;
	TransferOfferStatus status = TransferOfferStatus::Pending;
};