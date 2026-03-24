#include "Game.h"
#include "GameEvents.h"
#include "RosterLoader.h"
#include <utility>

Game::~Game() = default;

Game::Game() : date(2025, Month::July, 1), league("Super Lig"), rules(LeagueRules::makeSuperLig()), seasonPlan(SeasonPlan::build(2025, rules)), transferRoom(league), state(GameState::PreSeason), 
    eventsQueue(), matchScheduler(), fixtureGenerator(), domainEventPublisher(), playMatchCommandHandler(league, domainEventPublisher), leagueProjection(league), 
    user(), timePaused(false), dateWasReset(false), currentBlockingEvent(nullptr) {
  
    //takimlari txt dosyasindan okudugumuz yer (gecici)
    const std::string rosterPath = R"(C:\Users\user\Desktop\FootballManager\out\build\x64-Debug\FM_UI\FM\FM\database.txt)";
    RosterLoader::loadFromFile(league, rosterPath);

    domainEventPublisher.subscribeMatchPlayed([this](const MatchPlayedEvent& event) { leagueProjection.onMatchPlayed(event); });

    updateState();         
    seasonStartChecks();
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
    if (league.isSeasonFixtureGenerated() &&!league.allMatchesPlayed() && !(date < kickoff) && date < nextPreseasonStart) {
        state = GameState::InSeason;
        return;
    }
    if(league.isSeasonFixtureGenerated() && league.allMatchesPlayed() && date < nextPreseasonStart){
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
   
    //gunluk mac kontrolu
    matchScheduler.update(*this, eventsQueue);

    while (!eventsQueue.empty()){
        auto item = eventsQueue.popNext();

        if (std::holds_alternative<PlayMatchCommand>(item)) {
            playMatchCommandHandler.handle(std::get<PlayMatchCommand>(item));
            continue;
        }
        auto event = std::move(std::get<std::unique_ptr<GameEvents>>(item));

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
    // Haftalýk otomatik event uretimi burada genisletilebilir
}

void Game::seasonStartChecks() {
    const int y = date.getYear();
    const Date preseasonStart = rules.preseasonStart(y);

    const bool isPreseasonStartToday =
        date.getYear() == preseasonStart.getYear() &&
        date.getMonth() == preseasonStart.getMonth() &&
        date.getDay() == preseasonStart.getDay();

    if (!isPreseasonStartToday) {
        return;
    }
    if (lastSeasonRolloverYear == y) {
        return;
    }
    lastSeasonRolloverYear = y;

    // Kontratlarý 1'er yil azalt
    transferRoom.updatePlayersContractYearsInTeams();

    // Lig yeni sezon reset
    league.resetForNewSeason(y);
    transferRoom.collectFreeAgentsFromTeams();

    // Plan + fixture
    seasonPlan = SeasonPlan::build(y, rules);
    fixtureGenerator.generateSeasonFixture(league, seasonPlan, rules);
    seasonPlan.finalizeFromFixture(league, rules);
    league.setSeasonFixtureGenerated(true);
}

void Game::seasonEndChecks() {
    
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

void Game::setUserTeam(const std::string& teamName) {
    user.setTeam(teamName);
}

GameState Game::getState() const {
    return state;
}

const LeagueRules& Game::getRules() const {
    return rules;
}

const SeasonPlan& Game::getSeasonPlan() const {
    return seasonPlan;
}

//debug
const MatchScheduler& Game::getMatchScheduler() const {
    return matchScheduler;
}