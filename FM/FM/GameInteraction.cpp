#include"GameInteraction.h"

GameInteraction::GameInteraction(Kind interactionKind, bool isBlockingInteraction) : kind(interactionKind), blocking(isBlockingInteraction), resolved(false) {}

GameInteraction::Kind GameInteraction::getKind() const {
	return kind;
}

bool GameInteraction::isBlocking() const {
    return blocking;
}

bool GameInteraction::isResolved() const {
    return resolved;
}

void GameInteraction::resolve() {
    resolved = true;
}