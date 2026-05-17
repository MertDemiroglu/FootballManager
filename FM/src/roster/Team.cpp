#include"fm/roster/Team.h"
#include"fm/roster/Footballer.h"
#include"fm/transfer/TransferRoom.h"
#include"fm/common/Types.h"
#include"fm/roster/Contract.h"
#include"fm/roster/Formation.h"
#include"fm/roster/HeadCoach.h"

#include<vector>
#include<string>
#include<memory>
#include<ostream>
#include<algorithm>
#include<iostream>
#include<atomic>
#include<unordered_map>
#include<cassert>
#include<stdexcept>
#include<utility>

namespace {
    constexpr const char* DefaultPrimaryColor = "#22c55e";
    constexpr const char* DefaultSecondaryColor = "#0f172a";

    std::atomic<TeamId>& getNextTeamIdCounter() {
        static std::atomic<TeamId> nextId{ 1 };
        return nextId;
    }

    TeamId generateTeamId() {
        return getNextTeamIdCounter().fetch_add(1);
    }

    void reserveTeamId(TeamId id) {
        auto& nextId = getNextTeamIdCounter();
        TeamId desiredNextId = id + 1;
        TeamId currentNextId = nextId.load();

        while (currentNextId < desiredNextId && !nextId.compare_exchange_weak(currentNextId, desiredNextId)) {
        }
    }

    std::string buildDefaultHeadCoachName(const std::string& teamName) {
        return teamName + " Head Coach";
    }
}
Team::Team(const std::string& name) : Team(generateTeamId(), name) {}

Team::Team(TeamId id, const std::string& name)
    : id(id),
      name(name),
      primaryColor(DefaultPrimaryColor),
      secondaryColor(DefaultSecondaryColor),
      headCoach(buildDefaultHeadCoachName(name), FormationId::FourFourTwo) {
    if (id == 0) {
        throw std::invalid_argument("team id cannot be zero");
    }
    if (name.empty()) {
        throw std::invalid_argument("team name cannot be empty");
    }
    reserveTeamId(id);
}

TeamId Team::getId() const {
    return id;
}

const std::string& Team::getName() const {
    return name;
}

const std::string& Team::getPrimaryColor() const {
    return primaryColor;
}

const std::string& Team::getSecondaryColor() const {
    return secondaryColor;
}

void Team::setTeamColors(const std::string& newPrimaryColor, const std::string& newSecondaryColor) {
    primaryColor = newPrimaryColor.empty() ? DefaultPrimaryColor : newPrimaryColor;
    secondaryColor = newSecondaryColor.empty() ? DefaultSecondaryColor : newSecondaryColor;
}

const HeadCoach& Team::getHeadCoach() const {
    return headCoach;
}

HeadCoach& Team::getHeadCoach() {
    return headCoach;
}

void Team::setHeadCoach(HeadCoach newHeadCoach) {
    headCoach = std::move(newHeadCoach);
}

bool Team::hasSelectedTeamSheet() const {
    return selectedTeamSheet.has_value();
}

const TeamSheet* Team::getSelectedTeamSheet() const {
    return selectedTeamSheet.has_value() ? &*selectedTeamSheet : nullptr;
}

TeamSheet* Team::getSelectedTeamSheet() {
    return selectedTeamSheet.has_value() ? &*selectedTeamSheet : nullptr;
}

void Team::setSelectedTeamSheet(TeamSheet teamSheet) {
    if (teamSheet.teamId != id) {
        throw std::invalid_argument("selected teamsheet team id does not match team");
    }

    selectedTeamSheet = std::move(teamSheet);
}

void Team::clearSelectedTeamSheet() {
    selectedTeamSheet.reset();
}

size_t Team::playerCount() const {
    return players.size();
}
int Team::calculateTeamRating() const {
    int total = 0;
    if (players.empty()) return 0;

    for (const auto& p : players) {
        total += p->totalPower();
    }
    return total / players.size();
}

Footballer* Team::findPlayerById(PlayerId playerId) {
    const auto it = playerIndexById.find(playerId);
    return it != playerIndexById.end() ? it->second : nullptr;
}

const Footballer* Team::findPlayerById(PlayerId playerId) const {
    const auto it = playerIndexById.find(playerId);
    return it != playerIndexById.end() ? it->second : nullptr;
}

const std::vector<std::unique_ptr<Footballer>>& Team::getPlayers() const {
    return players;
}

void Team::rebuildPlayerIndexById() {
    playerIndexById.clear();
    playerIndexById.reserve(players.size());

    for (const auto& player : players) {
        if (!player) {
            continue;
        }
        playerIndexById.emplace(player->getId(), player.get());
    }
}

void Team::addPlayer(std::unique_ptr<Footballer> player) {
    if (!player) {
        return;
    }
    const PlayerId playerId = player->getId();
    if (playerId == 0) {
        throw std::invalid_argument("player id cannot be zero");
    }
    if (playerIndexById.find(playerId) != playerIndexById.end()) {
        throw std::logic_error("duplicate player id in team");
    }

    player->setTeam(id);
    players.push_back(std::move(player));

    Footballer* insertedPlayer = players.back().get();

    try {
        const auto [it, inserted] = playerIndexById.emplace(playerId, insertedPlayer);
        if (!inserted) {
            players.pop_back();
            throw std::logic_error("duplicate player id in team index");
        }
        (void)it;
    }
    catch (...) {
        if (!players.empty() && players.back().get() == insertedPlayer) {
            players.pop_back();
        }
        throw;
    }
    assert(playerIndexById.size() <= players.size());
}

std::unique_ptr<Footballer> Team::releasePlayer(PlayerId playerId) {
    Footballer* indexedPlayer = findPlayerById(playerId);
    if (!indexedPlayer) {
        return nullptr;
    }

     auto it = std::find_if(players.begin(), players.end(), [indexedPlayer](const std::unique_ptr<Footballer>& p) {
        return p.get() == indexedPlayer;
        });

    if (it == players.end()) {
        rebuildPlayerIndexById();
        return nullptr;
    }

    std::unique_ptr<Footballer> released = std::move(*it);
    released->setTeam(0);
    playerIndexById.erase(playerId);
    players.erase(it);
    assert(playerIndexById.size() <= players.size());
    return released;
}

//transfer islemleri
bool Team::canAffordTransfer(Money amount) const {
    return finance.canAffordTransfer(amount);
}

bool Team::canAffordWage(Money amount) const {
    return finance.canFitWage(calculateCurrentAnnualWageSpend(), amount);
}
void Team::earn(Money amount) {
    finance.receiveCash(amount);
}

void Team::spend(Money amount) {
    (void)finance.spendCash(amount);
}

void Team::setBudgetSnapshot(Money totalBudgetValue, Money transferBudgetValue, Money wageBudgetValue) {
    finance.setSnapshot(totalBudgetValue, transferBudgetValue, wageBudgetValue, finance.getStrategy());
}

void Team::setBudgetSnapshot(
    Money totalBudgetValue,
    Money transferBudgetValue,
    Money wageBudgetValue,
    ClubFinancialStrategy strategy) {
    finance.setSnapshot(totalBudgetValue, transferBudgetValue, wageBudgetValue, strategy);
}

Money Team::getTotalBudget() const {
    return finance.getCashBalance();
}

Money Team::getTransferBudget() const {
    return finance.getTransferBudget();
}

Money Team::getWageBudget() const {
    return finance.getWageBudgetLimit();
}

ClubFinancialStrategy Team::getFinancialStrategy() const {
    return finance.getStrategy();
}

void Team::spendTransferFee(Money fee) {
    (void)finance.spendTransferFee(fee);
}

void Team::receiveTransferFee(Money fee) {
    finance.receiveTransferFee(fee);
}

Money Team::calculateCurrentAnnualWageSpend() const {
    Money totalWages = 0;
    for (const auto& player : players) {
        if (!player) {
            continue;
        }
        if (const Contract* contract = player->getContract()) {
            totalWages += contract->getWage();
        }
    }
    return totalWages;
}

Money Team::getAvailableWageBudget() const {
    return std::max<Money>(0, getWageBudget() - calculateCurrentAnnualWageSpend());
}

void Team::setBudgets() {
    finance.allocateFromCashBalance(finance.getCashBalance(), finance.getStrategy());
}
void Team::payWagesMonthly() {
   finance.payWages(calculateCurrentAnnualWageSpend() / 12);
}
std::vector<std::unique_ptr<Footballer>> Team::collectExpiredContracts() {
    //sozlesme bitimi kontrolu
    std::vector<std::unique_ptr<Footballer>> leavingPlayers;
    auto it = players.begin();
    while (it != players.end()) {
       auto c = (*it)->getContract();
       if (c && c->isExpired()) {
           playerIndexById.erase((*it)->getId());
           (*it)->setTeam(0);
           leavingPlayers.push_back(std::move(*it));
           it = players.erase(it);
       }
       else {
           ++it;
       }
    }
    if (playerIndexById.size() != players.size()) {
        rebuildPlayerIndexById();
    }
    assert(playerIndexById.size() <= players.size());
    return leavingPlayers;
}
//Kontrat yilini 1 azaltir (her oyuncu icin)
void Team::updateContracts() {
    for (const auto& p : players) {
        p->advanceContractYear();
    }
}

//player stats fonksiyonlari
void Team::resetPlayerSeasonStats(int newSeasonYear) {
    for (const auto& p : players) {
        p->initializeSeasonStats(newSeasonYear);
    }
}

void Team::archiveAndResetPlayerSeasonStats(int newSeasonYear) {
    for (const auto& p : players) {
        p->resetSeasonStats(newSeasonYear);
    }
}


//printler
std::ostream& operator<<(std::ostream& os, const Team& team) {
    team.print(os);
    return os;
}
void Team::print(std::ostream& os) const {
    os << "Team: " << name << "\n";
    for (const auto& p : players) {
        os << "  - " << *p << "\n";
    }
}
