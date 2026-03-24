#pragma once

#include"DomainEventPublisher.h"
#include"League.h"
#include"PlayMatchCommand.h"

class PlayMatchCommandHandler {
public:
	PlayMatchCommandHandler(League& league, DomainEventPublisher& publisher);
	void handle(const PlayMatchCommand& command);

private:
	League& league;
	DomainEventPublisher& publisher;
};