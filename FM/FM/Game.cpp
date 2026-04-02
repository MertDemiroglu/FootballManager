#include"Game.h"
#include"RosterLoader.h"

#include<stdexcept>
#include<utility>

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
    dateWasReset(false) {
    League league("Super Lig");
    LeagueRules rules = LeagueRules::makeSuperLig();
    SeasonPlan seasonPlan = SeasonPlan::build(2025, rules);


    //takimlari txt dosyasindan okudugumuz yer (gecici)
    const std::string rosterPath = R"(C:\Users\user\Desktop\FootballManager\out\build\x64-Debug\FM_UI\FM\FM\database.txt)";
    RosterLoader::loadFromFile(league, rosterPath);

    world.addLeagueContext(std::move(league), std::move(rules), std::move(seasonPlan), domainEventPublisher);

    domainEventPublisher.subscribeMatchPlayed([this](const MatchPlayedEvent& event) {
        LeagueContext* context = world.findLeagueContext(event.leagueId);
        if (!context) {
            throw std::logic_error("match played event references unknown league context");
        }
        context->getLeagueProjection().onMatchPlayed(event);
    });

    updateState();         
    seasonStartChecks();
    updateTransferWindow(); 
}

void Game::updateState() {

    bool anyInSeason = false;
    bool anyPostSeason = false;
    bool anyContext = false;

    world.forEachLeagueContext([&](const LeagueContext& context) {
        anyContext = true;


        const League& league = context.getLeague();
        const SeasonPlan& seasonPlan = context.getSeasonPlan();

        const Date& preseasonStart = seasonPlan.getPreseasonStart();
        const Date& kickoff = seasonPlan.getKickoff();
        const Date& nextPreseasonStart = seasonPlan.getNextPreseasonStart();

        const bool isPreseason = !(date < preseasonStart) && date < kickoff;
        const bool isInSeason = league.isSeasonFixtureGenerated() && !league.allMatchesPlayed() && !(date < kickoff) && date < nextPreseasonStart;
        const bool isPostSeason = league.isSeasonFixtureGenerated() && league.allMatchesPlayed() && date < nextPreseasonStart;

        if (isInSeason) {
            anyInSeason = true;
            return;
        }
        if (isPostSeason) {
            anyPostSeason = true;
            return;
        }
        (void)isPreseason;
    });

        if (!anyContext) {
            throw std::logic_error("game world has no league contexts");
        }


        if (anyInSeason) {
            state = GameState::InSeason;
            return;
        }
        if (anyPostSeason) {
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
    matchScheduler.update(world, date, eventsQueue);

    while (!eventsQueue.empty()) {
        const PlayMatchCommand command = eventsQueue.popNext();
        LeagueContext* context = world.findLeagueContext(command.leagueId);
        if (!context) {
            throw std::logic_error("play command references unknown league context");
        }
        context->getPlayMatchCommandHandler().handle(context->getLeague(), command);
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

    world.forEachLeagueContext([&](LeagueContext& context) {
        if (context.getLeague().allMatchesPlayed()) {
            seasonEndChecksForContext(context);
        }
        });
}

void Game::handleMonthlyEvents() {
    world.forEachLeagueContext([&](LeagueContext& context) {
        League& league = context.getLeague();
        for (auto& [teamId, team] : league.getTeams()) {
            (void)teamId;
            team->payWagesMonthly();
        }
        });
}

void Game::handleWeeklyEvents() {
    // Haftalik otomatik event uretimi burada genisletilebilir
}

void Game::seasonStartChecks() {
    world.forEachLeagueContext([&](LeagueContext& context) {
        seasonStartChecksForContext(context);
        });
}

void Game::seasonStartChecksForContext(LeagueContext& context) {

    League& league = context.getLeague();
    TransferRoom& transferRoom = world.getTransferRoom();
    LeagueRules& rules = context.getRules();
    SeasonPlan& seasonPlan = context.getSeasonPlan();

    const int y = date.getYear();
    const Date preseasonStart = rules.preseasonStart(y);

    const bool isPreseasonStartToday =
        date.getYear() == preseasonStart.getYear() &&
        date.getMonth() == preseasonStart.getMonth() &&
        date.getDay() == preseasonStart.getDay();

    if(!isPreseasonStartToday) {
        return;
    }
    if(context.getLastSeasonRolloverYear() == y) {
        return;
    }
    context.setLastSeasonRolloverYear(y);

    // Kontratlari 1'er yil azalt
    transferRoom.updatePlayersContractYearsInLeague(league.getId());

    // Lig yeni sezon reset
    league.resetForNewSeason(y);
    transferRoom.collectFreeAgentsFromLeague(league.getId());

    // Plan + fixture
    seasonPlan = SeasonPlan::build(y, rules);
    fixtureGenerator.generateSeasonFixture(league, seasonPlan, rules);
    seasonPlan.finalizeFromFixture(league, rules);
    league.setSeasonFixtureGenerated(true);
}

void Game::seasonEndChecks() {
    world.forEachLeagueContext([&](LeagueContext& context) {
        seasonEndChecksForContext(context);
        });
}

void Game::seasonEndChecksForContext(LeagueContext& context) {
    (void)context;
}

void Game::updateTransferWindow() {

    world.forEachLeagueContext([&](LeagueContext& context) {
        const SeasonPlan& seasonPlan = context.getSeasonPlan();
        TransferRoom& transferRoom = world.getTransferRoom();

        const LeagueId leagueId = context.getLeague().getId();
        if (seasonPlan.getSummerWindow().contains(date) || seasonPlan.getWinterWindow().contains(date)) {
            transferRoom.openWindowForLeague(leagueId);
        }
        else {
            transferRoom.closeWindowForLeague(leagueId);
        }
        });
}

void Game::stopTime() {
    timePaused = true;
    //Zamanin o an ki ilerleyisi burada durdurulacak ve event oyuncuya sunulacak, oyuncu tekrar baslatana kadar zaman duracak
}

bool Game::isTimePaused() const {
    return timePaused;
}

void Game::processBlockingEvent() {
 
    timePaused = false;
}

Date& Game::getDate() {
    return date;
}

const Date& Game::getDate() const {
    return date;
}


League* Game::findLeagueById(LeagueId id) {
    return world.findLeagueById(id);
}

const League* Game::findLeagueById(LeagueId id) const {
    return world.findLeagueById(id);
}

LeagueContext* Game::findLeagueContextById(LeagueId id) {
    return world.findLeagueContext(id);
}

const LeagueContext* Game::findLeagueContextById(LeagueId id) const {
    return world.findLeagueContext(id);
}

void Game::forEachLeagueContext(const std::function<void(LeagueContext&)>& visitor) {
    world.forEachLeagueContext(visitor);
}

void Game::forEachLeagueContext(const std::function<void(const LeagueContext&)>& visitor) const {
    world.forEachLeagueContext(visitor);
}

void Game::setUserTeam(LeagueId leagueId, TeamId teamId) {
    LeagueContext* context = world.findLeagueContext(leagueId);
    if (!context) {
        throw std::logic_error("cannot set user team for unknown league");
    }
    if (!context->getLeague().hasTeam(teamId)) {
        throw std::logic_error("cannot set user team for unknown team in league");
    }
    user.setTeam(leagueId, teamId);
}

GameState Game::getState() const {
    return state;
}


//debug
const MatchScheduler& Game::getMatchScheduler() const {
    return matchScheduler;
}