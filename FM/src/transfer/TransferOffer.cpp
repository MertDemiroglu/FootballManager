#include"fm/transfer/TransferOffer.h"

#include<stdexcept>

std::string toStableCode(TransferOfferStatus status) {
    switch (status) {
    case TransferOfferStatus::Pending:
        return "pending";
    case TransferOfferStatus::Resolved:
        return "resolved";
    }

    throw std::runtime_error("cannot persist unsupported transfer offer status");
}

std::optional<TransferOfferStatus> transferOfferStatusFromStableCode(const std::string& stableCode) {
    if (stableCode == "pending") {
        return TransferOfferStatus::Pending;
    }
    if (stableCode == "resolved") {
        return TransferOfferStatus::Resolved;
    }
    return std::nullopt;
}

std::string toStableCode(TransferOfferResolution resolution) {
    switch (resolution) {
    case TransferOfferResolution::Accepted:
        return "accepted";
    case TransferOfferResolution::Rejected:
        return "rejected";
    case TransferOfferResolution::ExpiredByDeadline:
        return "expired_by_deadline";
    case TransferOfferResolution::ExpiredByWindowClose:
        return "expired_by_window_close";
    }

    throw std::runtime_error("cannot persist unsupported transfer offer resolution");
}

std::optional<TransferOfferResolution> transferOfferResolutionFromStableCode(const std::string& stableCode) {
    if (stableCode == "accepted") {
        return TransferOfferResolution::Accepted;
    }
    if (stableCode == "rejected") {
        return TransferOfferResolution::Rejected;
    }
    if (stableCode == "expired_by_deadline") {
        return TransferOfferResolution::ExpiredByDeadline;
    }
    if (stableCode == "expired_by_window_close") {
        return TransferOfferResolution::ExpiredByWindowClose;
    }
    return std::nullopt;
}

std::string toStableCode(TransferOfferExpiryPolicy expiryPolicy) {
    switch (expiryPolicy) {
    case TransferOfferExpiryPolicy::FourteenDayLimit:
        return "fourteen_day_limit";
    case TransferOfferExpiryPolicy::WindowCloseLimit:
        return "window_close_limit";
    }

    throw std::runtime_error("cannot persist unsupported transfer offer expiry policy");
}

std::optional<TransferOfferExpiryPolicy> transferOfferExpiryPolicyFromStableCode(const std::string& stableCode) {
    if (stableCode == "fourteen_day_limit") {
        return TransferOfferExpiryPolicy::FourteenDayLimit;
    }
    if (stableCode == "window_close_limit") {
        return TransferOfferExpiryPolicy::WindowCloseLimit;
    }
    return std::nullopt;
}
