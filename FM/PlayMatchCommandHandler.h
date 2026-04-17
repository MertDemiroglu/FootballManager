#pragma once

#include"DomainEventPublisher.h"
#include"League.h"
#include"PlayMatchCommand.h"
#include "TeamSheet.h"

class PlayMatchCommandHandler {
public:
	PlayMatchCommandHandler(DomainEventPublisher& publisher);
	void handle(League& league, const PlayMatchCommand& command);
    void handle(League& league, const PlayMatchCommand& command, const TeamSheet& homeSheet, const TeamSheet& awaySheet);

private:
	DomainEventPublisher& publisher;
};
