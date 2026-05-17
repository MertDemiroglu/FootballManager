#include "fm/finance/TeamFinance.h"

#include <algorithm>
#include <stdexcept>

namespace {
    struct StrategyPolicy {
        int wageAllocationPercent;
        int transferAllocationPercent;
        int saleRetentionPercent;
    };

    StrategyPolicy policyFor(ClubFinancialStrategy strategy) {
        switch (strategy) {
        case ClubFinancialStrategy::Balanced:
            return { 60, 40, 65 };
        case ClubFinancialStrategy::DevelopmentFocused:
            return { 55, 45, 80 };
        case ClubFinancialStrategy::StarFocused:
            return { 75, 25, 60 };
        case ClubFinancialStrategy::ValueTrading:
            return { 50, 50, 85 };
        case ClubFinancialStrategy::Conservative:
            return { 50, 35, 50 };
        }

        throw std::runtime_error("unsupported club financial strategy");
    }

    void validateNonNegative(Money value, const char* fieldName) {
        if (value < 0) {
            throw std::invalid_argument(std::string(fieldName) + " cannot be negative");
        }
    }
}

std::string toStableCode(ClubFinancialStrategy strategy) {
    switch (strategy) {
    case ClubFinancialStrategy::Balanced:
        return "balanced";
    case ClubFinancialStrategy::DevelopmentFocused:
        return "development_focused";
    case ClubFinancialStrategy::StarFocused:
        return "star_focused";
    case ClubFinancialStrategy::ValueTrading:
        return "value_trading";
    case ClubFinancialStrategy::Conservative:
        return "conservative";
    }

    throw std::runtime_error("cannot persist unsupported club financial strategy");
}

std::optional<ClubFinancialStrategy> clubFinancialStrategyFromStableCode(const std::string& stableCode) {
    if (stableCode == "balanced") {
        return ClubFinancialStrategy::Balanced;
    }
    if (stableCode == "development_focused") {
        return ClubFinancialStrategy::DevelopmentFocused;
    }
    if (stableCode == "star_focused") {
        return ClubFinancialStrategy::StarFocused;
    }
    if (stableCode == "value_trading") {
        return ClubFinancialStrategy::ValueTrading;
    }
    if (stableCode == "conservative") {
        return ClubFinancialStrategy::Conservative;
    }
    return std::nullopt;
}

std::string toDisplayText(ClubFinancialStrategy strategy) {
    switch (strategy) {
    case ClubFinancialStrategy::Balanced:
        return "Balanced";
    case ClubFinancialStrategy::DevelopmentFocused:
        return "Development focused";
    case ClubFinancialStrategy::StarFocused:
        return "Star focused";
    case ClubFinancialStrategy::ValueTrading:
        return "Value trading";
    case ClubFinancialStrategy::Conservative:
        return "Conservative";
    }

    throw std::runtime_error("cannot display unsupported club financial strategy");
}

void TeamFinance::capTransferBudgetToCashBalance() {
    if (transferBudget > cashBalance) {
        transferBudget = cashBalance;
    }
}

void TeamFinance::setSnapshot(
    Money newCashBalance,
    Money newTransferBudget,
    Money newWageBudgetLimit,
    ClubFinancialStrategy newStrategy) {
    validateNonNegative(newCashBalance, "cash balance");
    validateNonNegative(newTransferBudget, "transfer budget");
    validateNonNegative(newWageBudgetLimit, "wage budget limit");

    cashBalance = newCashBalance;
    transferBudget = newTransferBudget;
    wageBudgetLimit = newWageBudgetLimit;
    strategy = newStrategy;
}

void TeamFinance::allocateFromCashBalance(Money startingCash, ClubFinancialStrategy newStrategy) {
    validateNonNegative(startingCash, "starting cash");

    const StrategyPolicy policy = policyFor(newStrategy);
    cashBalance = startingCash;
    transferBudget = startingCash * policy.transferAllocationPercent / 100;
    wageBudgetLimit = startingCash * policy.wageAllocationPercent / 100;
    strategy = newStrategy;
}

bool TeamFinance::canAffordTransfer(Money fee) const {
    return fee >= 0 && fee <= transferBudget && fee <= cashBalance;
}

bool TeamFinance::spendTransferFee(Money fee) {
    validateNonNegative(fee, "transfer fee");
    if (!canAffordTransfer(fee)) {
        return false;
    }

    cashBalance -= fee;
    transferBudget -= fee;
    return true;
}

void TeamFinance::receiveTransferFee(Money fee) {
    validateNonNegative(fee, "transfer fee");

    const StrategyPolicy policy = policyFor(strategy);
    cashBalance += fee;
    transferBudget += fee * policy.saleRetentionPercent / 100;
    capTransferBudgetToCashBalance();
}

bool TeamFinance::canFitWage(Money currentAnnualWageSpend, Money newAnnualWage) const {
    return currentAnnualWageSpend >= 0
        && newAnnualWage >= 0
        && currentAnnualWageSpend + newAnnualWage <= wageBudgetLimit;
}

bool TeamFinance::spendCash(Money amount) {
    validateNonNegative(amount, "cash spend");
    if (amount > cashBalance) {
        return false;
    }

    cashBalance -= amount;
    capTransferBudgetToCashBalance();
    return true;
}

void TeamFinance::receiveCash(Money amount) {
    validateNonNegative(amount, "cash receipt");
    cashBalance += amount;
}

void TeamFinance::payWages(Money monthlyWageTotal) {
    validateNonNegative(monthlyWageTotal, "monthly wage total");
    cashBalance = std::max<Money>(0, cashBalance - monthlyWageTotal);
    capTransferBudgetToCashBalance();
}

Money TeamFinance::getCashBalance() const {
    return cashBalance;
}

Money TeamFinance::getTransferBudget() const {
    return transferBudget;
}

Money TeamFinance::getWageBudgetLimit() const {
    return wageBudgetLimit;
}

ClubFinancialStrategy TeamFinance::getStrategy() const {
    return strategy;
}
