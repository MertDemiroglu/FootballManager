#include"TransferOfferEvent.h"
#include"Team.h"
#include"Game.h"

TransferOfferEvent::TransferOfferEvent(const std::string& s, const std::string& b, const std::string& p, Money f) : sellingTeamName(s), buyingTeamName(b), playerName(p), fee(f){}

bool TransferOfferEvent::isBlocking() const { 
    return block;
}

void TransferOfferEvent::resolve(Game& game) {
    Team* sellingTeam = game.getLeague().getTeam(sellingTeamName);
    Team* buyingTeam = game.getLeague().getTeam(buyingTeamName);

    if (!sellingTeam || !buyingTeam) {
        return;
    }

    Footballer* player = sellingTeam->findPlayer(playerName);

    if (!player) {
        return;
    }

    game.getTransferRoom().transferPlayer(sellingTeam->getName(), buyingTeam->getName(), player->getName(), fee);
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