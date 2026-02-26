#include"EventQueue.h"
#include"GameEvents.h"

std::unique_ptr<GameEvents> EventQueue::popEvent() {
    if (events.empty()) return nullptr;

    auto it = std::max_element(
        events.begin(),
        events.end(),
        [](const auto& a, const auto& b) {
            return static_cast<int>(a->getPriority())
                < static_cast<int>(b->getPriority());
        }
    );

    auto event = std::move(*it);
    events.erase(it);
    return event;
}
void EventQueue::pushEvent(std::unique_ptr<GameEvents> event) {
    events.push_back(std::move(event));
}
bool EventQueue::empty() const {
    return events.empty();
}