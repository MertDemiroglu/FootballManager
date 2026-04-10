#include"Game.h"
#include"Team.h"
#include"League.h"
#include"RosterLoader.h"
#include"PostMatchInteraction.h"
#include"TransferOfferDecisionInteraction.h"

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
    interactionManager(),
    user(),
    timePaused(false),
    userPaused(false),
    dateWasReset(false) {
    League league("Super Lig");
    LeagueRules rules = LeagueRules::makeSuperLig();
    SeasonPlan seasonPlan = SeasonPlan::build(2025, rules);


    //takimlari txt dosyasindan okudugumuz yer (gecici)
    const std::string rosterPath = R"(C:\Users\user\Desktop\FootballManager\out\build\x64-Debug\FM_UI\FM\FM\database.txt)";
    RosterLoader::loadFromFile(league, rosterPath);

    world.addLeagueContext(std::move(league), std::move(rules), std::move(seasonPlan));

    world.getDomainEventPublisher().subscribeMatchPlayed([this](const MatchPlayedEvent& event) {
        LeagueContext* context = world.findLeagueContext(event.leagueId);
        if (!context) {
            throw std::logic_error("match played event references unknown league context");
        }
        context->getLeagueProjection().onMatchPlayed(event);
    });

    world.getDomainEventPublisher().subscribeMatchPlayed([this](const MatchPlayedEvent& event) {
        const LeagueId managedLeagueId = user.getManagedLeagueId();
        const TeamId managedTeamId = user.getManagedTeamId();
        if (managedLeagueId == 0 || managedTeamId == 0) {
            return;
        }
        if (event.leagueId != managedLeagueId) {
            return;
        }
        if (event.homeId != managedTeamId && event.awayId != managedTeamId) {
            return;
        }

        interactionManager.enqueue(std::make_unique<PostMatchInteraction>(
            event.leagueId,
            event.date,
            event.homeId,
            event.awayId,
            event.homeGoals,
            event.awayGoals,
            event.matchweek));
        refreshTimePauseState();
        });

    world.getDomainEventPublisher().subscribeTransferOfferCreated([this](const TransferOfferCreatedEvent& event) {
        const LeagueId managedLeagueId = user.getManagedLeagueId();
        const TeamId managedTeamId = user.getManagedTeamId();
        if (managedLeagueId == 0 || managedTeamId == 0) {
            return;
        }
        if (event.sellerLeagueId != managedLeagueId || event.sellerTeamId != managedTeamId) {
            return;
        }

        interactionManager.enqueue(std::make_unique<TransferOfferDecisionInteraction>(
            event.offerId,
            event.sellerLeagueId,
            event.sellerTeamId,
            event.buyerLeagueId,
            event.buyerTeamId,
            event.playerId,
            event.fee));
        refreshTimePauseState();
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
    refreshTimePauseState();

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

        refreshTimePauseState();
        if (timePaused) {
            return;
        }
    }

    handleSeasonalEvents();
    refreshTimePauseState();
    if (timePaused) {
        return;
    }

    updateTransferWindow();
    refreshTimePauseState();
    if (timePaused) {
        return;
    }

    if (date.isNewWeek()) {
        handleWeeklyEvents();
        refreshTimePauseState();
        if (timePaused) {
            return;
        }
    }

    if (date.isNewMonth()) { 
        handleMonthlyEvents();
        refreshTimePauseState();
        if (timePaused) {
            return;
        }
    }

    updateState();
    refreshTimePauseState();
    if (timePaused) {
        return;
    }
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
    temporaryForDebug_tryCreateWeeklyManagedTransferOffer();
}

void Game::temporaryForDebug_tryCreateWeeklyManagedTransferOffer() {
    const LeagueId managedLeagueId = user.getManagedLeagueId();
    const TeamId managedTeamId = user.getManagedTeamId();
    if (managedLeagueId == 0 || managedTeamId == 0) {
        return;
    }

    if (!world.getTransferRoom().isOpenForLeague(managedLeagueId)) {
        return;
    }

    LeagueContext* context = world.findLeagueContext(managedLeagueId);
    if (!context) {
        return;
    }

    League& league = context->getLeague();
    Team* sellerTeam = league.findTeamById(managedTeamId);
    if (!sellerTeam || sellerTeam->getPlayers().empty()) {
        return;
    }

    TeamId buyerTeamId = 0;
    for (const auto& [candidateTeamId, candidateTeam] : league.getTeams()) {
        (void)candidateTeam;
        if (candidateTeamId == managedTeamId) {
            continue;
        }
        if (buyerTeamId == 0 || candidateTeamId < buyerTeamId) {
            buyerTeamId = candidateTeamId;
        }
    }

    if (buyerTeamId == 0) {
        return;
    }

    const PlayerId playerId = sellerTeam->getPlayers().front()->getId();
    constexpr Money weeklyRuntimeOfferFee = 1;
    world.getTransferOfferService().createOffer(
        date,
        managedLeagueId,
        managedTeamId,
        managedLeagueId,
        buyerTeamId,
        playerId,
        weeklyRuntimeOfferFee);
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

bool Game::pauseSimulation() {
    userPaused = true;
    refreshTimePauseState();
    return true;
}

bool Game::resumeSimulation() {
    if (hasActiveBlockingInteraction()) {
        return false;
    }
    userPaused = false;
    refreshTimePauseState();
    return true;
}

void Game::processBlockingEvent() {
    refreshTimePauseState();
}

void Game::refreshTimePauseState() {
    timePaused = userPaused || interactionManager.hasActiveBlockingInteraction();
}

bool Game::hasActiveBlockingInteraction() const {
    return interactionManager.hasActiveBlockingInteraction();
}

const GameInteraction* Game::getActiveInteraction() const {
    return interactionManager.getActiveInteraction();
}

const PostMatchInteraction* Game::getActivePostMatchInteraction() const {
    return dynamic_cast<const PostMatchInteraction*>(interactionManager.getActiveInteraction());
}

const TransferOfferDecisionInteraction* Game::getActiveTransferOfferDecisionInteraction() const {
    return dynamic_cast<const TransferOfferDecisionInteraction*>(interactionManager.getActiveInteraction());
}

bool Game::resolveActiveInteraction() {
    const bool resolved = interactionManager.resolveActiveInteraction();
    refreshTimePauseState();
    return resolved;
}

bool Game::acceptTransferOffer(OfferId offerId) {
    if (offerId == 0) {
        return false;
    }

    const TransferOfferDecisionInteraction* interaction = getActiveTransferOfferDecisionInteraction();
    if (!interaction) {
        return false;
    }

    if (interaction->getOfferId() != offerId) {
        return false;
    }

    if (!world.getTransferOfferService().acceptOffer(offerId, date)) {
        return false;
    }

    interactionManager.resolveActiveInteraction();
    refreshTimePauseState();
    return true;
}

bool Game::rejectTransferOffer(OfferId offerId) {
    if (offerId == 0) {
        return false;
    }

    const TransferOfferDecisionInteraction* interaction = getActiveTransferOfferDecisionInteraction();
    if (!interaction) {
        return false;
    }

    if (interaction->getOfferId() != offerId) {
        return false;
    }

    if (!world.getTransferOfferService().rejectOffer(offerId)) {
        return false;
    }

    interactionManager.resolveActiveInteraction();
    refreshTimePauseState();
    return true;
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