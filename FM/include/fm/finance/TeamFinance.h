#pragma once

#include "fm/common/Types.h"

#include <optional>
#include <string>

enum class ClubFinancialStrategy {
    Balanced,
    DevelopmentFocused,
    StarFocused,
    ValueTrading,
    Conservative
};

std::string toStableCode(ClubFinancialStrategy strategy);
std::optional<ClubFinancialStrategy> clubFinancialStrategyFromStableCode(const std::string& stableCode);
std::string toDisplayText(ClubFinancialStrategy strategy);

class TeamFinance {
private:
    Money cashBalance = 0;
    Money transferBudget = 0;
    Money wageBudgetLimit = 0;
    ClubFinancialStrategy strategy = ClubFinancialStrategy::Balanced;

    void capTransferBudgetToCashBalance();

public:
    TeamFinance() = default;

    void setSnapshot(
        Money cashBalance,
        Money transferBudget,
        Money wageBudgetLimit,
        ClubFinancialStrategy strategy);
    void allocateFromCashBalance(Money startingCash, ClubFinancialStrategy strategy);

    bool canAffordTransfer(Money fee) const;
    bool spendTransferFee(Money fee);
    void receiveTransferFee(Money fee);

    bool canFitWage(Money currentAnnualWageSpend, Money newAnnualWage) const;
    bool spendCash(Money amount);
    void receiveCash(Money amount);
    void payWages(Money monthlyWageTotal);

    Money getCashBalance() const;
    Money getTransferBudget() const;
    Money getWageBudgetLimit() const;
    ClubFinancialStrategy getStrategy() const;
};
