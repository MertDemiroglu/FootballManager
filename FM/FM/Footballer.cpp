#include"Footballer.h"

#include<atomic>

namespace {
    PlayerId generatePlayerId() {
        static std::atomic<PlayerId> nextId{ 1 };
        return nextId++;
    }

}
Footballer::Footballer(const std::string& name, const std::string& position, const std::string& team, int age) : playerId(generatePlayerId()), name(name), position(position), team(team), teamId(0), age(age) {}

PlayerId Footballer::getId() const {
    return playerId;
}

TeamId Footballer::getTeamId() const {
    return teamId;
}

//get fonksiyonlari

const std::string& Footballer::getName() const {
    return name;
}

const std::string& Footballer::getPosition() const {
    return position;
}

const std::string& Footballer::getTeam() const {
    return team;
}

int Footballer::getAge() const {
    return age;
}

//set fonksiyonlari

void Footballer::setTeam(const std::string& newTeam, TeamId newTeamId) {
    team = newTeam;
    teamId = newTeamId;

    if (contract) {
        contract->setTeamId(newTeamId);
    }
}

//contract fonksiyonlari

void Footballer::signContract(Money wage, int years) {
    contract = std::make_unique<Contract>(playerId, teamId, wage, years);
}
const Contract* Footballer::getContract() const{
    return contract.get();
}

//print

void Footballer::print(std::ostream& os) const {
    os << "Name: " << name << ", Id: " << playerId << ", Age: " << age << ", Position: " << position << ", Team: " << team;
}


std::ostream& operator<<(std::ostream& os, const Footballer& f) {
    f.print(os);
    return os;
}

void Footballer::advanceContractYear() {
    if (contract) {
        contract->advanceYear();
    }
}