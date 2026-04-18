#pragma once

#include<cstddef>
#include<deque>
#include<memory>

#include"fm/interaction/GameInteraction.h"

class InteractionManager {
private:
    void activateNextPendingIfNeeded();

    std::deque<std::unique_ptr<GameInteraction>> pendingInteractions;
    std::unique_ptr<GameInteraction> activeInteraction;

public:
	void enqueue(std::unique_ptr<GameInteraction> interaction);

	bool hasActiveInteraction() const;
	bool hasActiveBlockingInteraction() const;


    const GameInteraction* getActiveInteraction() const;
    GameInteraction* getActiveInteraction();

    std::size_t pendingCount() const;

    bool resolveActiveInteraction();
};