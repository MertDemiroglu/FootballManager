#pragma once

#include"DomainEventPublisher.h"
#include"League.h"
#include"PlayMatchCommand.h"

class PlayMatchCommandHandler {
public:
	PlayMatchCommandHandler(DomainEventPublisher& publisher);
	void handle(League& league, const PlayMatchCommand& command);

private:
	DomainEventPublisher& publisher;
};