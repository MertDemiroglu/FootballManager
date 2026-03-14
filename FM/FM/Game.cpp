#include "Game.h"
#include "GameEvents.h"
#include "RosterLoader.h"

Game::~Game() = default;

Game::Game()
    : date(2025, Month::July, 1),
    league("Super Lig"),
    rules(LeagueRules::makeSuperLig()),
    seasonPlan(SeasonPlan::build(2025, rules)),
    transferRoom(league),
    state(GameState::PreSeason),
    eventsQueue(),
    user(),
    timePaused(false),
    dateWasReset(false),
    currentBlockingEvent(nullptr) {
    RosterLoader::loadFromFile(league, "database.txt");
    seasonStartChecks();
    updateState();
    updateTransferWindow();
}

void Game::updateState() {
    const Date& preseasonStart = seasonPlan.getPreseasonStart();
    const Date& kickoff = seasonPlan.getKickoff();
    const Date& nextPreseasonStart = seasonPlan.getNextPreseasonStart();

    if (!(date < preseasonStart) && date < kickoff) {
        state = GameState::PreSeason;
        return;
    }

    if (league.isSeasonFixtureGenerated() && !league.allMatchesPlayed() && !(date < kickoff)) {
        state = GameState::InSeason;
        return;
    }

    if (league.isSeasonFixtureGenerated() && league.allMatchesPlayed() && date < nextPreseasonStart) {
        state = GameState::PostSeason;
        return;
    }

    state = GameState::PreSeason;
}

void Game::updateDaily() {

    if (timePaused) {
        processBlockingEvent();
        return;
    }

    matchScheduler.update(*this, eventsQueue);

    while (!eventsQueue.empty()){
        auto event = eventsQueue.popEvent();
        if (!event) {
            continue;
        }

        Team* userTeam = nullptr;
        const std::string managedTeam = user.getTeam();
        if (!managedTeam.empty()) {
            userTeam = league.getTeam(managedTeam);
        }
        if(event->isBlocking() && userTeam && event->affectsTeam(userTeam)){
            currentBlockingEvent = std::move(event);
            stopTime();
            return;
        }
        event->resolve(*this);
    }
    handleSeasonalEvents();
    updateTransferWindow();

    if (date.isNewWeek()) {
        handleWeeklyEvents();
    }
    if (date.isNewMonth()) {
        handleMonthlyEvents();
    }

    updateState();

    if (!dateWasReset) {
        date.advanceDay();
    }
    else {
        dateWasReset = false;
    }
}

void Game::handleSeasonalEvents() {
    const Date& nextPre = seasonPlan.getNextPreseasonStart();
    if (date.getYear() == nextPre.getYear() && date.getMonth() == nextPre.getMonth() && date.getDay() == nextPre.getDay()) {
        league.resetForNewSeason();
    }

    seasonStartChecks();

    if (league.allMatchesPlayed()) {
        seasonEndChecks();
    }
}

void Game::handleMonthlyEvents() {
     for (auto& [name, team] : league.getTeams()) {
         (void)name;
         team->payWagesMonthly();
     }
}

void Game::handleWeeklyEvents() {
}

void Game::seasonStartChecks() {
    if (date.getDay() == seasonPlan.getPreseasonStart().getDay()
        && date.getMonth() == seasonPlan.getPreseasonStart().getMonth()
        && date.getYear() == seasonPlan.getPreseasonStart().getYear()
        && (!league.isSeasonFixtureGenerated())) {

        transferRoom.collectFreeAgentsFromTeams();

        seasonPlan = SeasonPlan::build(date.getYear(), rules);
        fixtureGenerator.generateSeasonFixture(league, seasonPlan, rules);
        seasonPlan.finalizeFromFixture(league, rules);
        league.setSeasonFixtureGenerated(true);
    }
}

void Game::seasonEndChecks() {
    transferRoom.updatePlayersContractYearsInTeams();
}

void Game::updateTransferWindow() {
    if (seasonPlan.getSummerWindow().contains(date) || seasonPlan.getWinterWindow().contains(date)) {
        transferRoom.openWindow();
    }
    else {
        transferRoom.closeWindow();
    }
}

TransferRoom& Game::getTransferRoom() {
    return transferRoom;
}

const TransferRoom& Game::getTransferRoom() const {
    return transferRoom;
}

void Game::stopTime() {
    timePaused = true;
}

bool Game::isTimePaused() const {
    return timePaused;
}

void Game::processBlockingEvent() {
    if (currentBlockingEvent) {
        currentBlockingEvent->resolve(*this);
        currentBlockingEvent.reset();
    }
    timePaused = false;
}

Date& Game::getDate() {
    return date;
}

const Date& Game::getDate() const {
    return date;
}

League& Game::getLeague() {
    return league;
}

const League& Game::getLeague() const {
    return league;
}

void Game::setUserTeam(const std::string& teamName) {
    user.setTeam(teamName);
}

const MatchScheduler& Game::getMatchScheduler() const {
    return matchScheduler;
}

GameState Game::getState() const {
    return state;
}
