#pragma once

#include <QObject>

#include "PostMatchInteractionObject.h"
#include "PreMatchInteractionObject.h"
#include "TransferOfferInteractionObject.h"

class InteractionStateObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(InteractionStateObject)

    Q_PROPERTY(bool hasActiveInteraction READ hasActiveInteraction NOTIFY changed)
    Q_PROPERTY(QString kind READ kind NOTIFY changed)
    Q_PROPERTY(PreMatchInteractionObject* preMatch READ preMatch CONSTANT)
    Q_PROPERTY(PostMatchInteractionObject* postMatch READ postMatch CONSTANT)
    Q_PROPERTY(TransferOfferInteractionObject* transferOffer READ transferOffer CONSTANT)

public:
    explicit InteractionStateObject(QObject* parent = nullptr);

    bool hasActiveInteraction() const;
    QString kind() const;

    PreMatchInteractionObject* preMatch() const;
    PostMatchInteractionObject* postMatch() const;
    TransferOfferInteractionObject* transferOffer() const;

    void setFromValues(bool hasActiveInteraction, const QString& kind);
    void clear();

signals:
    void changed();

private:
    bool hasActiveInteractionValue = false;
    QString kindValue;
    PreMatchInteractionObject preMatchObject;
    PostMatchInteractionObject postMatchObject;
    TransferOfferInteractionObject transferOfferObject;
};
