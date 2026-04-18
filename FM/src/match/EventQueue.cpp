#include"fm/match/EventQueue.h"
#include<stdexcept>

PlayMatchCommand EventQueue::popNext() {
    if (items.empty()) {
        throw std::logic_error("event queue is empty");
    }
    PlayMatchCommand item = items.front();
    items.erase(items.begin());
    return item;
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