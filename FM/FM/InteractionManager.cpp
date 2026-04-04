#include "InteractionManager.h"

void InteractionManager::enqueue(std::unique_ptr<GameInteraction> interaction) {
    if (!interaction) {
        return;
    }
    pendingInteractions.push_back(std::move(interaction));
    activateNextPendingIfNeeded();
}

bool InteractionManager::hasActiveInteraction() const {
    return static_cast<bool>(activeInteraction);
}

bool InteractionManager::hasActiveBlockingInteraction() const {
    return activeInteraction && activeInteraction->isBlocking() && !activeInteraction->isResolved();
}

const GameInteraction* InteractionManager::getActiveInteraction() const {
    return activeInteraction.get();
}

GameInteraction* InteractionManager::getActiveInteraction() {
    return activeInteraction.get();
}

std::size_t InteractionManager::pendingCount() const {
    return pendingInteractions.size();
}

bool InteractionManager::resolveActiveInteraction() {
    if (!activeInteraction) {
        return false;
    }

    activeInteraction->resolve();
    activeInteraction.reset();
    activateNextPendingIfNeeded();
    return true;
}

void InteractionManager::activateNextPendingIfNeeded() {
    if (activeInteraction || pendingInteractions.empty()) {
        return;
    }
    activeInteraction = std::move(pendingInteractions.front());
    pendingInteractions.pop_front();
}