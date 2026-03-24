#pragma once

#include<functional>
#include<vector>

#include"MatchPlayedEvent.h"

class DomainEventPublisher {
public:
	using MatchPlayedSubscriber = std::function<void(const MatchPlayedEvent&)>;

	void subscribeMatchPlayed(MatchPlayedSubscriber subscriber);
	void publish(const MatchPlayedEvent& event) const;

private:
	std::vector<MatchPlayedSubscriber> matchPlayedSubscribers;
};
