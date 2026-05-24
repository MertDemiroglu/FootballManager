#pragma once

#include"fm/events/DomainEventPublisher.h"
#include"fm/competition/League.h"
#include"fm/match/PlayMatchCommand.h"
#include"fm/match/TeamSheet.h"

enum class MatchSimulationEngineMode {
    Lightweight,
    Coordinate
};

struct PlayMatchCommandHandlerOptions {
    MatchSimulationEngineMode engineMode = MatchSimulationEngineMode::Coordinate;
};

class PlayMatchCommandHandler {
public:
	PlayMatchCommandHandler(
        DomainEventPublisher& publisher,
        PlayMatchCommandHandlerOptions options = PlayMatchCommandHandlerOptions{});
	void handle(League& league, const PlayMatchCommand& command);
    void handle(League& league, const PlayMatchCommand& command, const TeamSheet& homeSheet, const TeamSheet& awaySheet);

private:
	DomainEventPublisher& publisher;
    PlayMatchCommandHandlerOptions options;
};
