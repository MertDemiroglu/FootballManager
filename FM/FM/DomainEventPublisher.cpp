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