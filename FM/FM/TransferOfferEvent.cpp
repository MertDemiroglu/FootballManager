#include"TransferOfferEvent.h"
#include"Team.h"
#include"Game.h"
#include"Types.h"

TransferOfferEvent::TransferOfferEvent(Team* s, Team* b, Footballer* p, Money f) : sellingTeam(s), buyingTeam(b), player(p), fee(f){}

bool TransferOfferEvent::isBlocking() const { return true; }

void TransferOfferEvent::resolve(Game& game) {
    game.getTransferRoom().transferPlayer(sellingTeam->getName(), buyingTeam->getName(), player->getName(), fee);
}
EventPriority TransferOfferEvent::getPriority() const {
    return priority;
}
EventTargetType TransferOfferEvent::getTargetType() const {
    return type;
}
Team* TransferOfferEvent::getReceivingTeam() const {
    return sellingTeam;
}
Team* TransferOfferEvent::getSendingTeam() const {
    return buyingTeam;
}
Footballer* TransferOfferEvent::getPlayer() const {
    return player;
}
bool TransferOfferEvent::affectsTeam(const Team* team) const {
    return team == sellingTeam;
}