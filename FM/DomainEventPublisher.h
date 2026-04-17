#pragma once

#include<functional>
#include<vector>

#include"MatchPlayedEvent.h"
#include"PlayerTransferredEvent.h"
#include"TransferOfferCreatedEvent.h"
#include"TransferOfferResolvedEvent.h"

class DomainEventPublisher {
public:
	using MatchPlayedSubscriber = std::function<void(const MatchPlayedEvent&)>;
	using TransferOfferCreatedSubscriber = std::function<void(const TransferOfferCreatedEvent&)>;
	using TransferOfferResolvedSubscriber = std::function<void(const TransferOfferResolvedEvent&)>;
	using PlayerTransferredSubscriber = std::function<void(const PlayerTransferredEvent&)>;


	void subscribeMatchPlayed(MatchPlayedSubscriber subscriber);
	void subscribeTransferOfferCreated(TransferOfferCreatedSubscriber subscriber);
	void subscribeTransferOfferResolved(TransferOfferResolvedSubscriber subscriber);
	void subscribePlayerTransferred(PlayerTransferredSubscriber subscriber);

	void publish(const MatchPlayedEvent& event) const;
	void publish(const TransferOfferCreatedEvent& event) const;
	void publish(const TransferOfferResolvedEvent& event) const;
	void publish(const PlayerTransferredEvent& event) const;

private:
	std::vector<MatchPlayedSubscriber> matchPlayedSubscribers;
	std::vector<TransferOfferCreatedSubscriber> transferOfferCreatedSubscribers;
	std::vector<TransferOfferResolvedSubscriber> transferOfferResolvedSubscribers;
	std::vector<PlayerTransferredSubscriber> playerTransferredSubscribers;
};
