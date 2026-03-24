#include"EventQueue.h"
#include"GameEvents.h"

EventQueue::QueueItem EventQueue::popNext() {
    if (items.empty()) {
        return std::unique_ptr<GameEvents>{};
    }
    QueueItem item = std::move(items.front());
    items.erase(items.begin());
    return item;
}

void EventQueue::pushEvent(std::unique_ptr<GameEvents> event) {
    if (!event) {
        return;
    }
    items.push_back(std::move(event));
}

void EventQueue::pushCommand(const PlayMatchCommand& command) {
    items.push_back(command);
}

bool EventQueue::empty() const {
    return items.empty();
}

std::size_t EventQueue::size() const {
    return items.size();
}