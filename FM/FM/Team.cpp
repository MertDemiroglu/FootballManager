#include"Team.h"
#include"Footballer.h"
#include"TransferRoom.h"
#include"Types.h"
#include"Contract.h"

#include<vector>
#include<string>
#include<memory>
#include<ostream>
#include<algorithm>
#include<iostream>
#include<atomic>

namespace {
    TeamId generateTeamId() {
        static std::atomic<TeamId> nextId{ 1 };
        return nextId++;
    }

}
Team::Team(const std::string& name) : Team(generateTeamId(), name) {}

Team::Team(TeamId id, const std::string& name) : id(id), name(name), wageBudget(0), transferBudget(0), totalBudget(0) {}

TeamId Team::getId() const {
    return id;
}

const std::string& Team::getName() const {
    return name;
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

//Takimda oyuncu bulur (isim ile bulup ptr verir)
Footballer* Team::findPlayer(const std::string& name) {
    for (auto& p : players) {
        if (p->getName() == name) {
            return p.get();
        }
    }
    return nullptr;
}

//Takima oyuncu ekler
void Team::addPlayer(std::unique_ptr<Footballer> player) {
    if (!player) {
        return;
    }
    players.push_back(std::move(player));
}

//Oyuncu serbest birakma fonksiyonu
std::unique_ptr<Footballer> Team::releasePlayer(const std::string& playerName) {
    //transferler icin
    auto it = std::find_if(players.begin(), players.end(),[&](const std::unique_ptr<Footballer>& p) {
            return p->getName() == playerName;
        });

    if (it == players.end())
        return nullptr;

    std::unique_ptr<Footballer> released = std::move(*it);

    players.erase(it);

    return released;
}

//transfer islemleri
bool Team::canAffordTransfer(Money amount) const {
    return transferBudget >= amount;
}

bool Team::canAffordWage(Money amount) const {
    return wageBudget >= amount;
}
void Team::earn(Money amount) {
    totalBudget += amount;
}

void Team::spend(Money amount) {
    totalBudget -= amount;
}

void Team::setBudgets() {
    wageBudget = totalBudget * 40 / 100;
    transferBudget = totalBudget * 60 / 100;
}
void Team::payWagesMonthly() {
   for (const auto& p : players) {
       //kontrol sistemi iyilestirilebilir
       if (const Contract* c = p->getContract()) {
           spend(c->getWage() / 12);
       }
   }  
}
std::vector<std::unique_ptr<Footballer>> Team::collectExpiredContracts() {
    //sozlesme bitimi kontrolu
    std::vector<std::unique_ptr<Footballer>> leavingPlayers;
    auto it = players.begin();
    while (it != players.end()) {
       auto c = (*it)->getContract();
       if (c && c->isExpired()) {
           leavingPlayers.push_back(std::move(*it));
           it = players.erase(it);
       }
       else {
           ++it;
       }
    }
    return leavingPlayers;
}
//Kontrat yilini 1 azaltir (her oyuncu icin)
void Team::updateContracts() {
    for (const auto& p : players) {
        p->advanceContractYear();
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
