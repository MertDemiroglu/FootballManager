#pragma once

#include <QObject>

class TransferOfferInteractionObject : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(TransferOfferInteractionObject)

    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(int offerId READ offerId NOTIFY changed)
    Q_PROPERTY(int playerId READ playerId NOTIFY changed)
    Q_PROPERTY(QString playerName READ playerName NOTIFY changed)
    Q_PROPERTY(QString sellerTeamName READ sellerTeamName NOTIFY changed)
    Q_PROPERTY(QString buyerTeamName READ buyerTeamName NOTIFY changed)
    Q_PROPERTY(qlonglong fee READ fee NOTIFY changed)

public:
    explicit TransferOfferInteractionObject(QObject* parent = nullptr);

    bool hasData() const;
    int offerId() const;
    int playerId() const;
    QString playerName() const;
    QString sellerTeamName() const;
    QString buyerTeamName() const;
    qlonglong fee() const;

    void setFromValues(int offerId,
        int playerId,
        const QString& playerName,
        const QString& sellerTeamName,
        const QString& buyerTeamName,
        qlonglong fee);
    void clear();

signals:
    void changed();

private:
    bool hasDataValue = false;
    int offerIdValue = 0;
    int playerIdValue = 0;
    QString playerNameValue;
    QString sellerTeamNameValue;
    QString buyerTeamNameValue;
    qlonglong feeValue = 0;
};
