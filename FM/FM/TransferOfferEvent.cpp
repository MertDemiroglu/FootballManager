#include"TransferOfferEvent.h"
#include"Game.h"

TransferOfferEvent::TransferOfferEvent(const std::string& s, const std::string& b, const std::string& p, Money f) : sellingTeamName(s), buyingTeamName(b), playerName(p), fee(f){}

bool TransferOfferEvent::isBlocking() const { 
    return block;
}

void TransferOfferEvent::resolve(Game& game) {
    game.getTransferRoom().transferPlayer(sellingTeamName, buyingTeamName, playerName, fee);
}

EventPriority TransferOfferEvent::getPriority() const {
    return priority;
}

EventTargetType TransferOfferEvent::getTargetType() const {
    return type;
}

const std::string& TransferOfferEvent::getReceivingTeam() const {
    return buyingTeamName;
}

const std::string& TransferOfferEvent::getSendingTeam() const {
    return sellingTeamName;
}

const std::string& TransferOfferEvent::getPlayer() const {
    return playerName;
}

bool TransferOfferEvent::affectsTeam(const Team* team) const {
    return team && team->getName() == sellingTeamName;
}