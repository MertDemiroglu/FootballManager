	#pragma once

#include<functional>
#include<vector>

#include"MatchPlayedEvent.h"
#include"TransferOfferCreatedEvent.h"

class DomainEventPublisher {
public:
	using MatchPlayedSubscriber = std::function<void(const MatchPlayedEvent&)>;
	using TransferOfferCreatedSubscriber = std::function<void(const TransferOfferCreatedEvent&)>;

	void subscribeMatchPlayed(MatchPlayedSubscriber subscriber);
	void subscribeTransferOfferCreated(TransferOfferCreatedSubscriber subscriber);
	void publish(const MatchPlayedEvent& event) const;
	void publish(const TransferOfferCreatedEvent& event) const;

private:
	std::vector<MatchPlayedSubscriber> matchPlayedSubscribers;
	std::vector<TransferOfferCreatedSubscriber> transferOfferCreatedSubscribers;
};
