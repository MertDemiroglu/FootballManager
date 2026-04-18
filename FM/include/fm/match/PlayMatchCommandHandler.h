#pragma once

#include"fm/events/DomainEventPublisher.h"
#include"fm/competition/League.h"
#include"fm/match/PlayMatchCommand.h"
#include"fm/match/TeamSheet.h"

class PlayMatchCommandHandler {
public:
	PlayMatchCommandHandler(DomainEventPublisher& publisher);
	void handle(League& league, const PlayMatchCommand& command);
    void handle(League& league, const PlayMatchCommand& command, const TeamSheet& homeSheet, const TeamSheet& awaySheet);

private:
	DomainEventPublisher& publisher;
};
