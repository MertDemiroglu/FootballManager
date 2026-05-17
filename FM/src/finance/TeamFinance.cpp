#include "fm/finance/TeamFinance.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace {
    struct StrategyPolicy {
        int wageSharePercent;
        int transferSharePercent;
        int saleRetentionModifierPercent;
    };

    struct HealthPolicy {
        int sportingAllocationPercent;
        int baseSaleRetentionPercent;
    };

    StrategyPolicy policyFor(ClubFinancialStrategy strategy) {
        switch (strategy) {
        case ClubFinancialStrategy::Balanced:
            return { 60, 40, 0 };
        case ClubFinancialStrategy::DevelopmentFocused:
            return { 55, 45, 5 };
        case ClubFinancialStrategy::StarFocused:
            return { 75, 25, 0 };
        case ClubFinancialStrategy::ValueTrading:
            return { 50, 50, 10 };
        case ClubFinancialStrategy::Conservative:
            return { 50, 35, -10 };
        }

        throw std::runtime_error("unsupported club financial strategy");
    }

    HealthPolicy policyFor(ClubFinancialHealth health) {
        switch (health) {
        case ClubFinancialHealth::Excellent:
            return { 85, 80 };
        case ClubFinancialHealth::Stable:
            return { 80, 70 };
        case ClubFinancialHealth::Cautious:
            return { 70, 55 };
        case ClubFinancialHealth::Tight:
            return { 55, 40 };
        case ClubFinancialHealth::Distressed:
            return { 40, 25 };
        }

        throw std::runtime_error("unsupported club financial health");
    }

    int saleRetentionPercent(ClubFinancialStrategy strategy, ClubFinancialHealth health) {
        const StrategyPolicy strategyPolicy = policyFor(strategy);
        const HealthPolicy healthPolicy = policyFor(health);
        return std::clamp(
            healthPolicy.baseSaleRetentionPercent + strategyPolicy.saleRetentionModifierPercent,
            10,
            90);
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

std::string toStableCode(ClubFinancialHealth health) {
    switch (health) {
    case ClubFinancialHealth::Excellent:
        return "excellent";
    case ClubFinancialHealth::Stable:
        return "stable";
    case ClubFinancialHealth::Cautious:
        return "cautious";
    case ClubFinancialHealth::Tight:
        return "tight";
    case ClubFinancialHealth::Distressed:
        return "distressed";
    }

    throw std::runtime_error("cannot persist unsupported club financial health");
}

std::optional<ClubFinancialHealth> clubFinancialHealthFromStableCode(const std::string& stableCode) {
    if (stableCode == "excellent") {
        return ClubFinancialHealth::Excellent;
    }
    if (stableCode == "stable") {
        return ClubFinancialHealth::Stable;
    }
    if (stableCode == "cautious") {
        return ClubFinancialHealth::Cautious;
    }
    if (stableCode == "tight") {
        return ClubFinancialHealth::Tight;
    }
    if (stableCode == "distressed") {
        return ClubFinancialHealth::Distressed;
    }
    return std::nullopt;
}

std::string toDisplayText(ClubFinancialHealth health) {
    switch (health) {
    case ClubFinancialHealth::Excellent:
        return "Excellent";
    case ClubFinancialHealth::Stable:
        return "Stable";
    case ClubFinancialHealth::Cautious:
        return "Cautious";
    case ClubFinancialHealth::Tight:
        return "Tight";
    case ClubFinancialHealth::Distressed:
        return "Distressed";
    }

    throw std::runtime_error("cannot display unsupported club financial health");
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
    ClubFinancialStrategy newStrategy,
    ClubFinancialHealth newHealth) {
    validateNonNegative(newCashBalance, "cash balance");
    validateNonNegative(newTransferBudget, "transfer budget");
    validateNonNegative(newWageBudgetLimit, "wage budget limit");

    cashBalance = newCashBalance;
    transferBudget = newTransferBudget;
    wageBudgetLimit = newWageBudgetLimit;
    strategy = newStrategy;
    health = newHealth;
}

void TeamFinance::allocateFromCashBalance(Money startingCash, ClubFinancialStrategy newStrategy) {
    allocateFromCashBalance(startingCash, newStrategy, health);
}

void TeamFinance::allocateFromCashBalance(
    Money startingCash,
    ClubFinancialStrategy newStrategy,
    ClubFinancialHealth newHealth) {
    validateNonNegative(startingCash, "starting cash");

    const StrategyPolicy strategyPolicy = policyFor(newStrategy);
    const HealthPolicy healthPolicy = policyFor(newHealth);
    const Money sportingEnvelope = startingCash * healthPolicy.sportingAllocationPercent / 100;
    cashBalance = startingCash;
    transferBudget = sportingEnvelope * strategyPolicy.transferSharePercent / 100;
    wageBudgetLimit = sportingEnvelope * strategyPolicy.wageSharePercent / 100;
    strategy = newStrategy;
    health = newHealth;
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

    cashBalance += fee;
    transferBudget += fee * saleRetentionPercent(strategy, health) / 100;
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

Money TeamFinance::calculateSportingAllocationEnvelope() const {
    return cashBalance * policyFor(health).sportingAllocationPercent / 100;
}

Money TeamFinance::getUnallocatedCashBuffer() const {
    const Money allocatedFootballBudget = transferBudget + wageBudgetLimit;
    if (cashBalance <= allocatedFootballBudget) {
        return 0;
    }
    return cashBalance - allocatedFootballBudget;
}

ClubFinancialStrategy TeamFinance::getStrategy() const {
    return strategy;
}

ClubFinancialHealth TeamFinance::getHealth() const {
    return health;
}
