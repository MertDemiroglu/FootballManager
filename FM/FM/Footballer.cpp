#include"Footballer.h"

Footballer::Footballer(const std::string& name, const std::string& position, const std::string& team, int age) : name(name), position(position), team(team), age(age){}

//get fonksiyonlarý

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

//set fonksiyonlarý

void Footballer::setTeam(const std::string& newTeam) {
    team = newTeam;
}

//contract fonksiyonlarý

void Footballer::signContract(Money wage, int years) {
    contract = std::make_unique<Contract>(wage, years);
}
const Contract* Footballer::getContract() const{
    return contract.get();
}

//print

void Footballer::print(std::ostream& os) const {
    os << "Name: " << name << ", Age: " << age<< ", Position: " << position << ", Team: " << team;
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