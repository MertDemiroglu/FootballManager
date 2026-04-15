#include"Footballer.h"

#include <atomic>
#include <stdexcept>

namespace {
    PlayerId generatePlayerId() {
        static std::atomic<PlayerId> nextId{ 1 };
        return nextId++;
    }

}
Footballer::Footballer(const std::string& name, PlayerPosition position, const std::string& team, int age)
    : playerId(generatePlayerId()), name(name), position(position), teamId(0), age(age) {
    (void)team;

    if (!isKnownPlayerPosition(position)) {
        throw std::invalid_argument("footballer position cannot be unknown");
    }

    currentSeasonStats.playerId = playerId;
}

Footballer::Footballer(const std::string& name, const std::string& position, const std::string& team, int age)
    : Footballer(name, parsePlayerPosition(position), team, age) {
}

//get fonksiyonlari
PlayerId Footballer::getId() const {
    return playerId;
}

TeamId Footballer::getTeamId() const {
    return teamId;
}

const std::string& Footballer::getName() const {
    return name;
}

std::string Footballer::getPosition() const {
    return toDisplayString(position);
}

PlayerPosition Footballer::getPlayerPosition() const {
    return position;
}

int Footballer::getAge() const {
    return age;
}

//set fonksiyonlari
void Footballer::setTeam(TeamId newTeamId) {
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

void Footballer::advanceContractYear() {
    if (contract) {
        contract->advanceYear();
    }
}

//player stats fonksiyonlari
const PlayerSeasonStats& Footballer::getCurrentSeasonStats() const {
    return currentSeasonStats;
}

const std::unordered_map<int, PlayerSeasonStats>& Footballer::getArchivedSeasonStatsByYear() const {
    return archivedSeasonStatsByYear;
}

void Footballer::initializeSeasonStats(int seasonYear) {
    if (seasonYear < 0) {
        throw std::invalid_argument("season year cannot be negative");
    }

    currentSeasonStats = PlayerSeasonStats{};
    currentSeasonStats.playerId = playerId;
    currentSeasonStats.seasonYear = seasonYear;
}

void Footballer::resetSeasonStats(int newSeasonYear) {
    archiveCurrentSeasonStats();
    initializeSeasonStats(newSeasonYear);
}



void Footballer::archiveCurrentSeasonStats() {
    if (currentSeasonStats.playerId == 0) {
        currentSeasonStats.playerId = playerId;
    }

    if (currentSeasonStats.seasonYear < 0) {
        throw std::logic_error("cannot archive player stats with invalid season year");
    }

    if (currentSeasonStats.seasonYear == 0 && archivedSeasonStatsByYear.empty()) {
        return;
    }

    auto [it, inserted] = archivedSeasonStatsByYear.emplace(currentSeasonStats.seasonYear, currentSeasonStats);
    if (!inserted) {
        throw std::logic_error("player season stats already archived");
    }
}

void Footballer::addAppearance(bool isStarter, int playedMinutes) {
    if (playedMinutes < 0) {
        throw std::invalid_argument("played minutes cannot be negative");
    }

    ++currentSeasonStats.appearances;
    if (isStarter) {
        ++currentSeasonStats.starts;
    }
    else {
        ++currentSeasonStats.substituteAppearances;
    }
    currentSeasonStats.minutesPlayed += playedMinutes;
}

void Footballer::addGoal() {
    ++currentSeasonStats.goals;
}

void Footballer::addAssist() {
    ++currentSeasonStats.assists;
}

void Footballer::addMinutes(int additionalMinutes) {
    if (additionalMinutes < 0) {
        throw std::invalid_argument("additional minutes cannot be negative");
    }
    currentSeasonStats.minutesPlayed += additionalMinutes;
}

void Footballer::addYellowCard() {
    ++currentSeasonStats.yellowCards;
}

void Footballer::addRedCard() {
    ++currentSeasonStats.redCards;
}

//print
void Footballer::print(std::ostream& os) const {
    os << "Name: " << name << ", Id: " << playerId << ", Age: " << age << ", Position: " << getPosition() << ", TeamId: " << teamId;
}

std::ostream& operator<<(std::ostream& os, const Footballer& f) {
    f.print(os);
    return os;
}
