#include"MatchEvent.h"
#include "Team.h"
#include "Game.h"


MatchEvent::MatchEvent(Team* h, Team* a) : home(h), away(a) {}

bool MatchEvent::isBlocking() const { 
    return block; 
}

EventPriority MatchEvent::getPriority() const {
    return priority;
}
void MatchEvent::resolve(Game& game) {
   
    //mac eventi burada islenecek
}
Team* MatchEvent::getSendingTeam() const {
    return home;
}
Team* MatchEvent::getReceivingTeam() const {
    return away;
}
EventTargetType MatchEvent::getTargetType() const {
    return type;
}
bool MatchEvent::affectsTeam(const Team* team) const {
    return team == home || team == away;
}