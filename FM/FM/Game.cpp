#include "Game.h"
#include "GameEvents.h"

void Game::updateState() {
    Month m = date.getMonth();

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
    
    while (!eventsQueue.empty()){

        auto event = eventsQueue.popEvent();

        if (event->isBlocking() && event->affectsTeam(league.getTeam(user.getTeam()))) {
            currentBlockingEvent = std::move(event);
            stopTime();
            return;
        }
        event->resolve(*this);
    }

    date.advanceDay();

    if (date.isNewMonth()) {
        handleMonthlyEvents();
        handleSeasonalEvents();
        updateState();
    }
    
}

void Game::handleSeasonalEvents() {
    switch (state) {
    case GameState::PreSeason:
        if (date.getMonth() == Month::July && date.getWeek() == 1) {
            seasonStartChecks();
        }
        break;

    case GameState::InSeason:
        //maçlar ve milyon tane event daha
        break;

    case GameState::PostSeason:
        //tüm maçlar bittikten sonra bu state geçilecek
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
   
}
void Game::seasonStartChecks() {
    transferRoom.collectFreeAgentsFromTeams();
}
void Game::seasonEndChecks() {
    transferRoom.updatePlayersContractYearsInTeams();
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
    //Zamanýn o an ki akýþý burada durdurulacak ve event oyuncuya sunulacak, oyuncu tekrar baþlatana kadar zaman akýþý duracak.
}
void Game::processBlockingEvent() {
    if (currentBlockingEvent) {
        currentBlockingEvent->resolve(*this);
        currentBlockingEvent.reset();
    }
    timePaused = false;
}