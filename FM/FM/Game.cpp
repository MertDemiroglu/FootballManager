#include "Game.h"
#include "GameEvents.h"
#include <utility>

Game::Game() : date(2025, Month::July, 1, 1), league("Super Lig"), transferRoom(league), state(GameState::PreSeason), eventsQueue(), user(), timePaused(false), currentBlockingEvent(nullptr) {
    updateState();           
    updateTransferWindow(); 
}

void Game::updateState() {
    const Month m = date.getMonth();

    if (m == Month::July || m == Month::August) {
        state = GameState::PreSeason;
    }
    else if (m == Month::September || m == Month::October || m == Month::November || m == Month::December || m == Month::January || m == Month::February || m == Month::March || m == Month::April || m == Month::May) {
        state = GameState::InSeason;
    }
    else {
        state = GameState::PostSeason;
    }
}

void Game::updateDaily() {

    if (timePaused) {
        processBlockingEvent();
        return;
    }
    //gunluk mac kontrolu
    matchScheduler.update(*this, eventsQueue);

    while (!eventsQueue.empty()){
        auto event = eventsQueue.popEvent();
        if (!event) {
            continue;
        }

        if (event->isBlocking() && event->affectsTeam(league.getTeam(user.getTeam()))) {
            currentBlockingEvent = std::move(event);
            stopTime();
            return;
        }
        event->resolve(*this);
    }

    date.advanceDay();
    updateTransferWindow();

    if (date.getWeek() % 7 == 1) {
        handleWeeklyEvents();
    }
    if (date.isNewMonth()) {
        handleMonthlyEvents();
        handleSeasonalEvents();
        updateState();
    }
    
}

void Game::handleSeasonalEvents() {
    switch (state) {
    case GameState::PreSeason:
        if (date.getMonth() == Month::July && date.getWeek() == 1 && date.getDay() == 1) {
            seasonStartChecks();
        }
        break;

    case GameState::InSeason:
        //maclar ve diger eventler burada planlanacak
        break;

    case GameState::PostSeason:
        //tum maclar bittikten sonra bu state'e gecilecek
        seasonEndChecks();
        break;


    }
}

void Game::handleMonthlyEvents() {      
     for (auto& [name, team] : league.getTeams()) {
            team->payWagesMonthly();
     }
}

void Game::handleWeeklyEvents() {
    // Haftalýk otomatik event uretimi burada genisletilebilir
}

void Game::seasonStartChecks() {
    transferRoom.collectFreeAgentsFromTeams();

    if (!league.isSeasonFixtureGenerated() && league.getTeams().size() == 18) {
        fixtureGenerator.generateSeasonFixture(league, date);
        league.setSeasonFixtureGenerated(true);
    }
}

void Game::seasonEndChecks() {
    transferRoom.updatePlayersContractYearsInTeams();
    league.resetForNewSeason();

    const int nextYear = date.getYear() + 1;
    date = Date(nextYear, Month::July, 1, 1);
    state = GameState::PreSeason;
}

void Game::updateTransferWindow() {
    if (date.isSummerTransferWindow() || date.isWinterTransferWindow()) {
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
    //Zamanin o an ki ilerleyisi burada durdurulacak ve event oyuncuya sunulacak, oyuncu tekrar baslatana kadar zaman duracak
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