#include "TransferOfferInteractionObject.h"

TransferOfferInteractionObject::TransferOfferInteractionObject(QObject* parent)
    : QObject(parent) {
}

bool TransferOfferInteractionObject::hasData() const {
    return hasDataValue;
}

int TransferOfferInteractionObject::offerId() const {
    return offerIdValue;
}

int TransferOfferInteractionObject::playerId() const {
    return playerIdValue;
}

QString TransferOfferInteractionObject::playerName() const {
    return playerNameValue;
}

QString TransferOfferInteractionObject::sellerTeamName() const {
    return sellerTeamNameValue;
}

QString TransferOfferInteractionObject::buyerTeamName() const {
    return buyerTeamNameValue;
}

qlonglong TransferOfferInteractionObject::fee() const {
    return feeValue;
}

void TransferOfferInteractionObject::setFromValues(int newOfferId,
    int newPlayerId,
    const QString& newPlayerName,
    const QString& newSellerTeamName,
    const QString& newBuyerTeamName,
    qlonglong newFee) {
    if (hasDataValue
        && offerIdValue == newOfferId
        && playerIdValue == newPlayerId
        && playerNameValue == newPlayerName
        && sellerTeamNameValue == newSellerTeamName
        && buyerTeamNameValue == newBuyerTeamName
        && feeValue == newFee) {
        return;
    }

    hasDataValue = true;
    offerIdValue = newOfferId;
    playerIdValue = newPlayerId;
    playerNameValue = newPlayerName;
    sellerTeamNameValue = newSellerTeamName;
    buyerTeamNameValue = newBuyerTeamName;
    feeValue = newFee;
    emit changed();
}

void TransferOfferInteractionObject::clear() {
    if (!hasDataValue
        && offerIdValue == 0
        && playerIdValue == 0
        && playerNameValue.isEmpty()
        && sellerTeamNameValue.isEmpty()
        && buyerTeamNameValue.isEmpty()
        && feeValue == 0) {
        return;
    }

    hasDataValue = false;
    offerIdValue = 0;
    playerIdValue = 0;
    playerNameValue.clear();
    sellerTeamNameValue.clear();
    buyerTeamNameValue.clear();
    feeValue = 0;
    emit changed();
}
