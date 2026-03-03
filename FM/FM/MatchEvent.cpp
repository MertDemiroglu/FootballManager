#include"MatchEvent.h"
#include "Team.h"
#include "Game.h"


MatchEvent::MatchEvent(std::string h, std::string a) : home(h), away(a) {}

bool MatchEvent::isBlocking() const { 
    return block; 
}

EventPriority MatchEvent::getPriority() const {
    return priority;
}
void MatchEvent::resolve(Game& game) {
   
    //mac eventi burada islenecek
}
const std::string& MatchEvent::getSendingTeam() const {
    return home;
}
const std::string& MatchEvent::getReceivingTeam() const {
    return away;
}
EventTargetType MatchEvent::getTargetType() const {
    return type;
}
bool MatchEvent::affectsTeam(const Team* team) const {
    if (!team) {
        return false;
    }
    const auto& name = team->getName();
    return name == home || name == away;
}

static const std::string kEmpty{};
const std::string& MatchEvent::getPlayer() const {
    return kEmpty;
}