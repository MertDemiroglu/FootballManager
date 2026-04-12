#include "PendingTransferOffersModel.h"

#include <utility>

PendingTransferOffersModel::PendingTransferOffersModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int PendingTransferOffersModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return rows.size();
}

QVariant PendingTransferOffersModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows.size()) {
        return {};
    }

    const Row& row = rows[static_cast<qsizetype>(index.row())];
    switch (role) {
    case OfferIdRole:
        return row.offerId;
    case PlayerIdRole:
        return row.playerId;
    case PlayerNameRole:
        return row.playerName;
    case BuyerTeamIdRole:
        return row.buyerTeamId;
    case BuyerTeamNameRole:
        return row.buyerTeamName;
    case FeeRole:
        return row.fee;
    case LastDateTextRole:
        return row.lastDateText;
    default:
        return {};
    }
}

QHash<int, QByteArray> PendingTransferOffersModel::roleNames() const {
    return {
        { OfferIdRole, "offerId" },
        { PlayerIdRole, "playerId" },
        { PlayerNameRole, "playerName" },
        { BuyerTeamIdRole, "buyerTeamId" },
        { BuyerTeamNameRole, "buyerTeamName" },
        { FeeRole, "fee" },
        { LastDateTextRole, "lastDateText" },
    };
}

void PendingTransferOffersModel::setRows(QVector<Row> newRows) {
    beginResetModel();
    rows = std::move(newRows);
    endResetModel();
}

void PendingTransferOffersModel::clear() {
    setRows({});
}
