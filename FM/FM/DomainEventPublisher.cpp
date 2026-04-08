#include"DomainEventPublisher.h"

void DomainEventPublisher::subscribeMatchPlayed(MatchPlayedSubscriber subscriber) {
	if (!subscriber) {
		return;
	}
	matchPlayedSubscribers.push_back(std::move(subscriber));
}

void DomainEventPublisher::subscribeTransferOfferCreated(TransferOfferCreatedSubscriber subscriber) {
	if (!subscriber) {
		return;
	}
	transferOfferCreatedSubscribers.push_back(std::move(subscriber));
}

void DomainEventPublisher::subscribeTransferOfferResolved(TransferOfferResolvedSubscriber subscriber) {
	if (!subscriber) {
		return;
	}
	transferOfferResolvedSubscribers.push_back(std::move(subscriber));
}

void DomainEventPublisher::subscribePlayerTransferred(PlayerTransferredSubscriber subscriber) {
	if (!subscriber) {
		return;
	}
	playerTransferredSubscribers.push_back(std::move(subscriber));
}


void DomainEventPublisher::publish(const MatchPlayedEvent& event) const {
	for (const auto& subscriber : matchPlayedSubscribers) {
		subscriber(event);
	}
}

void DomainEventPublisher::publish(const TransferOfferCreatedEvent& event) const {
	for (const auto& subscriber : transferOfferCreatedSubscribers) {
		subscriber(event);
	}
}

void DomainEventPublisher::publish(const TransferOfferResolvedEvent& event) const {
	for (const auto& subscriber : transferOfferResolvedSubscribers) {
		subscriber(event);
	}
}

void DomainEventPublisher::publish(const PlayerTransferredEvent& event) const {
	for (const auto& subscriber : playerTransferredSubscribers) {
		subscriber(event);
	}
}