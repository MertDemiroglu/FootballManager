#include "Game.h"
#include "GameEvents.h"
#include "RosterLoader.h"
#include <utility>

Game::~Game() = default;

Game::Game()
    : date(2025, Month::July, 1),
    world(),
    state(GameState::PreSeason),
    eventsQueue(),
    matchScheduler(),
    fixtureGenerator(),
    domainEventPublisher(),
    user(),
    timePaused(false),
    dateWasReset(false),
    currentBlockingEvent(nullptr) {
    League league("Super Lig");
    LeagueRules rules = LeagueRules::makeSuperLig();
    SeasonPlan seasonPlan = SeasonPlan::build(2025, rules);


    //takimlari txt dosyasindan okudugumuz yer (gecici)
    const std::string rosterPath = R"(C:\Users\user\Desktop\FootballManager\out\build\x64-Debug\FM_UI\FM\FM\database.txt)";
    RosterLoader::loadFromFile(league, rosterPath);

    LeagueContext& context = world.addLeagueContext(std::move(league), std::move(rules), std::move(seasonPlan), domainEventPublisher);

    domainEventPublisher.subscribeMatchPlayed([&context](const MatchPlayedEvent& event) {
        context.getLeagueProjection().onMatchPlayed(event);
        });

    updateState();         
    seasonStartChecks();
    updateTransferWindow(); 
}

void Game::updateState() {

    const LeagueContext& context = world.getPrimaryLeagueContext();
    const League& league = context.getLeague();
    const SeasonPlan& seasonPlan = context.getSeasonPlan();

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
            const PlayMatchCommand& command = std::get<PlayMatchCommand>(item);
            LeagueContext* context = world.findLeagueContext(command.leagueId);
            if (!context) {
                continue;
            }
            context->getPlayMatchCommandHandler().handle(context->getLeague(), command);
            continue;
        }
        auto event = std::move(std::get<std::unique_ptr<GameEvents>>(item));

        if (!event) {
            continue;
        }

        League& league = world.getPrimaryLeagueContext().getLeague();

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

    if (world.getPrimaryLeagueContext().getLeague().allMatchesPlayed()) {
        seasonEndChecks();
    }
}

void Game::handleMonthlyEvents() {
    League& league = world.getPrimaryLeagueContext().getLeague();
    for (auto& [name, team] : league.getTeams()) {
        (void)name;
        team->payWagesMonthly();
    }
}

void Game::handleWeeklyEvents() {
    // Haftalik otomatik event uretimi burada genisletilebilir
}

void Game::seasonStartChecks() {

    LeagueContext& context = world.getPrimaryLeagueContext();
    League& league = context.getLeague();
    TransferRoom& transferRoom = context.getTransferRoom();
    LeagueRules& rules = context.getRules();
    SeasonPlan& seasonPlan = context.getSeasonPlan();

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

    // Kontratlari 1'er yil azalt
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

    LeagueContext& context = world.getPrimaryLeagueContext();
    const SeasonPlan& seasonPlan = context.getSeasonPlan();
    TransferRoom& transferRoom = context.getTransferRoom();

    if (seasonPlan.getSummerWindow().contains(date) || seasonPlan.getWinterWindow().contains(date)) {
        transferRoom.openWindow();
    }
    else {
        transferRoom.closeWindow();
    }
}

TransferRoom& Game::getTransferRoom() {
    return world.getPrimaryLeagueContext().getTransferRoom();
}

const TransferRoom& Game::getTransferRoom() const {
    return world.getPrimaryLeagueContext().getTransferRoom();
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
    return world.getPrimaryLeagueContext().getLeague();
}

const League& Game::getLeague() const {
    return world.getPrimaryLeagueContext().getLeague();
}

League* Game::findLeagueById(LeagueId id) {
    return world.findLeagueById(id);
}

const League* Game::findLeagueById(LeagueId id) const {
    return world.findLeagueById(id);
}

void Game::setUserTeam(const std::string& teamName) {
    user.setTeam(teamName);
}

GameState Game::getState() const {
    return state;
}

const LeagueRules& Game::getRules() const {
    return world.getPrimaryLeagueContext().getRules();
}

const SeasonPlan& Game::getSeasonPlan() const {
    return world.getPrimaryLeagueContext().getSeasonPlan();
}

//debug
const MatchScheduler& Game::getMatchScheduler() const {
    return matchScheduler;
}