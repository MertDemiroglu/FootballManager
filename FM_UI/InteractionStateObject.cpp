#include "InteractionStateObject.h"

InteractionStateObject::InteractionStateObject(QObject* parent)
    : QObject(parent),
      preMatchObject(this),
      postMatchObject(this),
      transferOfferObject(this) {
}

bool InteractionStateObject::hasActiveInteraction() const {
    return hasActiveInteractionValue;
}

QString InteractionStateObject::kind() const {
    return kindValue;
}

PreMatchInteractionObject* InteractionStateObject::preMatch() const {
    return const_cast<PreMatchInteractionObject*>(&preMatchObject);
}

PostMatchInteractionObject* InteractionStateObject::postMatch() const {
    return const_cast<PostMatchInteractionObject*>(&postMatchObject);
}

TransferOfferInteractionObject* InteractionStateObject::transferOffer() const {
    return const_cast<TransferOfferInteractionObject*>(&transferOfferObject);
}

void InteractionStateObject::setFromValues(bool newHasActiveInteraction, const QString& newKind) {
    if (hasActiveInteractionValue == newHasActiveInteraction && kindValue == newKind) {
        return;
    }

    hasActiveInteractionValue = newHasActiveInteraction;
    kindValue = newKind;
    emit changed();
}

void InteractionStateObject::clear() {
    setFromValues(false, QString());
    preMatchObject.clear();
    postMatchObject.clear();
    transferOfferObject.clear();
}
