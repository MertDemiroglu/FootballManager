#pragma once

#include <QAbstractListModel>
#include <QVector>

class PendingTransferOffersModel : public QAbstractListModel {
    Q_OBJECT

public:
    struct Row {
        int offerId = 0;
        int playerId = 0;
        QString playerName;
        int buyerTeamId = 0;
        QString buyerTeamName;
        qlonglong fee = 0;
        QString lastDateText;
    };

    enum Role {
        OfferIdRole = Qt::UserRole + 1,
        PlayerIdRole,
        PlayerNameRole,
        BuyerTeamIdRole,
        BuyerTeamNameRole,
        FeeRole,
        LastDateTextRole,
    };

    explicit PendingTransferOffersModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QVector<Row> rows);
    void clear();

private:
    QVector<Row> rows;
};
