#include"fm/core/Game.h"

#include"fm/roster/Team.h"
#include"fm/roster/Footballer.h"

#include"fm/competition/League.h"

#include"fm/data/WorldBootstrapService.h"
#include"fm/data/SqliteGameStateRepository.h"
#include"fm/data/SqliteSaveMetadataRepository.h"
#include"fm/interaction/PostMatchInteraction.h"

#include"fm/interaction/TransferOfferDecisionInteraction.h"
#include"fm/interaction/PreMatchInteraction.h"
#include"fm/transfer/TransferOffer.h"
#include"fm/match/TeamSelectionService.h"
#include"fm/match/TeamSheet.h"
#include"fm/match/PlayerConditionService.h"

#include<filesystem>
#include<iomanip>
#include<sstream>
#include<stdexcept>
#include<unordered_map>
#include<unordered_set>
#include<utility>

namespace {
    LeagueRules buildDefaultBootstrapLeagueRules() {
        return LeagueRules::makeSuperLig();
    }

    SeasonPlan buildDefaultBootstrapSeasonPlan(const LeagueRules& rules) {
        return SeasonPlan::build(2025, rules);
    }

    std::string dateToIsoString(const Date& date) {
        std::ostringstream output;
        output << std::setw(4) << std::setfill('0') << date.getYear() << "-"
            << std::setw(2) << std::setfill('0') << static_cast<int>(date.getMonth()) << "-"
            << std::setw(2) << std::setfill('0') << date.getDay();
        return output.str();
    }

    bool sqliteDatabaseFileExists(const std::string& dbPath) {
        std::error_code error;
        const bool exists = std::filesystem::exists(dbPath, error);
        if (error) {
            throw std::invalid_argument("failed to inspect sqlite db path '" + dbPath + "': " + error.message());
        }
        return exists;
    }

    void requireSqliteSeedAssets(const GameBootstrapOptions& bootstrapOptions, const std::string& context) {
        if (bootstrapOptions.sqliteSchemaPath.empty()) {
            throw std::invalid_argument("sqlite schema path cannot be empty " + context);
        }
        if (bootstrapOptions.sqliteSeedPath.empty()) {
            throw std::invalid_argument("sqlite seed path cannot be empty " + context);
        }
    }

    void bootstrapWorldFromSqlite(World& world, const GameBootstrapOptions& bootstrapOptions, const LeagueRules& rules, const SeasonPlan& seasonPlan) {
        if (bootstrapOptions.sqliteDbPath.empty()) {
            throw std::invalid_argument("sqlite db path cannot be empty");
        }

        switch (bootstrapOptions.databaseOpenMode) {
        case DatabaseOpenMode::OpenExisting:
            if (!sqliteDatabaseFileExists(bootstrapOptions.sqliteDbPath)) {
                throw std::invalid_argument("sqlite db path does not exist for OpenExisting mode: " + bootstrapOptions.sqliteDbPath);
            }
            WorldBootstrapService::loadIntoWorldFromSqlite(world, bootstrapOptions.sqliteDbPath, rules, seasonPlan);
            return;
        case DatabaseOpenMode::CreateFromSeedIfMissing:
            if (sqliteDatabaseFileExists(bootstrapOptions.sqliteDbPath)) {
                WorldBootstrapService::loadIntoWorldFromSqlite(world, bootstrapOptions.sqliteDbPath, rules, seasonPlan);
                return;
            }
            requireSqliteSeedAssets(bootstrapOptions, "when creating missing sqlite database from seed");
            WorldBootstrapService::initializeAndLoadIntoWorldFromSqlite(
                world,
                bootstrapOptions.sqliteDbPath,
                bootstrapOptions.sqliteSchemaPath,
                bootstrapOptions.sqliteSeedPath,
                rules,
                seasonPlan);
            return;
        case DatabaseOpenMode::ResetFromSeed:
            requireSqliteSeedAssets(bootstrapOptions, "when resetting sqlite database from seed");
            WorldBootstrapService::resetAndLoadIntoWorldFromSqlite(
                world,
                bootstrapOptions.sqliteDbPath,
                bootstrapOptions.sqliteSchemaPath,
                bootstrapOptions.sqliteSeedPath,
                rules,
                seasonPlan);
            return;
        }

        throw std::invalid_argument("unsupported database open mode");
    }
}

void Game::ensureSaveMetadata(const GameBootstrapOptions& bootstrapOptions) {
    if (bootstrapOptions.mode != GameBootstrapMode::Sqlite || bootstrapOptions.sqliteDbPath.empty()) {
        saveMetadataEnabled = false;
        return;
    }

    saveMetadataDbPath = bootstrapOptions.sqliteDbPath;
    saveMetadataEnabled = true;

    SqliteSaveMetadataRepository repository(saveMetadataDbPath);
    if (!repository.exists()) {
        SaveMetadata defaultMetadata;
        defaultMetadata.saveSlotId = bootstrapOptions.saveSlotId;
        defaultMetadata.saveName = bootstrapOptions.saveName;
        defaultMetadata.managerName = "Manager";
        defaultMetadata.managedLeagueId = user.getManagedLeagueId();
        defaultMetadata.managedTeamId = user.getManagedTeamId();
        defaultMetadata.currentDate = dateToIsoString(date);
        defaultMetadata.schemaVersion = 2;
        defaultMetadata.worldVersion = 1;
        repository.insertDefault(defaultMetadata);
    }

    saveMetadata = repository.load();
}

void Game::updateManagedClubSaveMetadata() {
    if (!saveMetadataEnabled) {
        return;
    }

    SqliteSaveMetadataRepository repository(saveMetadataDbPath);
    repository.updateManagedClub(user.getManagedLeagueId(), user.getManagedTeamId());
    repository.touchUpdatedAt();
    saveMetadata = repository.load();
}

void Game::updateCurrentDateSaveMetadata() {
    if (!saveMetadataEnabled) {
        return;
    }

    SqliteSaveMetadataRepository repository(saveMetadataDbPath);
    repository.updateCurrentDate(dateToIsoString(date));
    repository.touchUpdatedAt();
    saveMetadata = repository.load();
}

void Game::restoreRuntimeState(const GameBootstrapOptions& bootstrapOptions) {
    if (!saveMetadataEnabled) {
        return;
    }

    SqliteGameStateRepository repository(bootstrapOptions.sqliteDbPath, GameStateRepositoryMode::ReadExisting);
    if (!repository.hasGameState()) {
        throw std::runtime_error("save slot is missing runtime game_state; clear incompatible development saves or start a new game");
    }

    date = repository.loadCurrentDate();

    const std::vector<PersistedLeagueRuntimeState> leagueStates = repository.loadLeagueRuntimeStates();
    const std::vector<PersistedFixtureState> fixtures = repository.loadFixtures();
    const std::vector<MatchReport> reports = repository.loadMatchReports();
    const std::vector<PersistedPlayerRuntimeState> playerStates = repository.loadPlayerRuntimeStates();

    if (leagueStates.empty()) {
        throw std::runtime_error("save slot is missing league runtime state");
    }
    if (fixtures.empty()) {
        throw std::runtime_error("save slot is missing persisted fixtures");
    }
    if (playerStates.empty()) {
        throw std::runtime_error("save slot is missing player runtime state");
    }

    std::unordered_map<LeagueId, PersistedLeagueRuntimeState> stateByLeague;
    for (const PersistedLeagueRuntimeState& state : leagueStates) {
        stateByLeague.emplace(state.leagueId, state);

        LeagueContext* context = world.findLeagueContext(state.leagueId);
        if (!context) {
            throw std::runtime_error("save slot references unknown league runtime state");
        }

        League& league = context->getLeague();
        LeagueRules& rules = context->getRules();
        SeasonPlan& seasonPlan = context->getSeasonPlan();

        context->setLastSeasonRolloverYear(state.lastSeasonRolloverYear);
        league.resetForNewSeason(state.seasonYear);
        league.initializeMatchdayTracking(rules.matchdaysPerSeason);
        seasonPlan = SeasonPlan::build(state.seasonYear, rules);
        league.setSeasonFixtureGenerated(false);
    }

    for (const PersistedFixtureState& fixture : fixtures) {
        const auto stateIt = stateByLeague.find(fixture.leagueId);
        if (stateIt == stateByLeague.end()) {
            throw std::runtime_error("persisted fixture references league without runtime state");
        }
        if (fixture.seasonYear != stateIt->second.seasonYear) {
            throw std::runtime_error("persisted fixture season year does not match league runtime state");
        }

        LeagueContext* context = world.findLeagueContext(fixture.leagueId);
        if (!context) {
            throw std::runtime_error("persisted fixture references unknown league");
        }

        League& league = context->getLeague();
        league.addFixtureMatch(
            fixture.matchId,
            fixture.matchweek,
            fixture.date,
            fixture.homeTeamId,
            fixture.awayTeamId);
        world.ensureNextMatchIdAfter(fixture.matchId);
    }

    for (const PersistedLeagueRuntimeState& state : leagueStates) {
        LeagueContext* context = world.findLeagueContext(state.leagueId);
        if (!context) {
            continue;
        }

        League& league = context->getLeague();
        SeasonPlan& seasonPlan = context->getSeasonPlan();
        if (state.fixtureGenerated && league.debugTotalFixtureMatches() > 0) {
            seasonPlan.finalizeFromFixture(league, context->getRules());
        }
        league.setSeasonFixtureGenerated(state.fixtureGenerated);
    }

    std::unordered_set<MatchId> restoredReportIds;
    for (const MatchReport& report : reports) {
        LeagueContext* context = world.findLeagueContext(report.leagueId);
        if (!context) {
            throw std::runtime_error("persisted match report references unknown league");
        }
        context->getLeague().restoreMatchReport(report);
        restoredReportIds.insert(report.matchId);
        world.ensureNextMatchIdAfter(report.matchId);
    }

    for (const PersistedFixtureState& fixture : fixtures) {
        if (!fixture.played || restoredReportIds.find(fixture.matchId) != restoredReportIds.end()) {
            continue;
        }

        LeagueContext* context = world.findLeagueContext(fixture.leagueId);
        if (!context) {
            throw std::runtime_error("persisted fixture result references unknown league");
        }
        context->getLeague().restoreFixtureResult(
            fixture.matchId,
            fixture.date,
            fixture.seasonYear,
            fixture.homeTeamId,
            fixture.awayTeamId,
            fixture.homeGoals,
            fixture.awayGoals,
            fixture.matchweek);
    }

    for (const PersistedPlayerRuntimeState& state : playerStates) {
        Footballer* player = nullptr;
        world.forEachLeagueContext([&](LeagueContext& context) {
            if (!player) {
                player = context.getLeague().findPlayerById(state.playerId);
            }
        });
        if (!player) {
            throw std::runtime_error("player runtime state references unknown player");
        }

        PlayerConditionState& condition = player->getConditionState();
        condition.setForm(state.form);
        condition.setFitness(state.fitness);
        condition.setMorale(state.morale);
    }

    SqliteSaveMetadataRepository metadataRepository(saveMetadataDbPath);
    metadataRepository.updateCurrentDate(dateToIsoString(date));
    saveMetadata = metadataRepository.load();
}

void Game::persistRuntimeState() {
    if (!saveMetadataEnabled) {
        return;
    }

    std::vector<PersistedLeagueRuntimeState> leagueStates;
    std::vector<PersistedFixtureState> fixtures;
    std::vector<MatchReport> reports;
    std::vector<PersistedPlayerRuntimeState> playerStates;

    world.forEachLeagueContext([&](const LeagueContext& context) {
        const League& league = context.getLeague();
        leagueStates.push_back(PersistedLeagueRuntimeState{
            league.getId(),
            league.getCurrentSeasonYear(),
            league.isSeasonFixtureGenerated(),
            context.getLastSeasonRolloverYear()
        });

        for (const auto& [fixtureDate, matches] : league.getFixture()) {
            for (const FixtureMatch& match : matches) {
                fixtures.push_back(PersistedFixtureState{
                    match.matchId,
                    league.getId(),
                    league.getCurrentSeasonYear(),
                    match.matchweek,
                    fixtureDate,
                    match.homeId,
                    match.awayId,
                    match.played,
                    false,
                    match.homeGoals,
                    match.awayGoals
                });
            }
        }

        for (const MatchRecord& record : league.getCurrentSeasonHistory()) {
            const MatchReport* report = league.findCurrentSeasonMatchReportById(record.matchId);
            if (report) {
                reports.push_back(*report);
            }
        }

        for (const auto& [teamId, team] : league.getTeams()) {
            (void)teamId;
            if (!team) {
                continue;
            }
            for (const auto& player : team->getPlayers()) {
                if (!player) {
                    continue;
                }
                const PlayerConditionState& condition = player->getConditionState();
                playerStates.push_back(PersistedPlayerRuntimeState{
                    player->getId(),
                    condition.getForm(),
                    condition.getFitness(),
                    condition.getMorale()
                });
            }
        }
    });

    // TODO: Transfer offer and roster mutation persistence needs dedicated runtime
    // tables; this repository covers date, fixtures/results, standings rebuild data,
    // match reports, and player condition for the current save/load gameplay scope.
    SqliteGameStateRepository repository(saveMetadataDbPath);
    repository.saveRuntimeState(date, static_cast<int>(state), leagueStates, fixtures, reports, playerStates);
}

Game::~Game() = default;

Game::Game(const GameBootstrapOptions& bootstrapOptions)
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
    dateWasReset(false),
    pendingPreMatchCommand(std::nullopt),
    pendingPreMatchHomeSheet(std::nullopt),
    pendingPreMatchAwaySheet(std::nullopt) {
    const LeagueRules rules = buildDefaultBootstrapLeagueRules();
    const SeasonPlan seasonPlan = buildDefaultBootstrapSeasonPlan(rules);

    bool loadedRuntimeState = false;

    switch (bootstrapOptions.mode) {
    case GameBootstrapMode::Sqlite:
        bootstrapWorldFromSqlite(world, bootstrapOptions, rules, seasonPlan);
        ensureSaveMetadata(bootstrapOptions);
        if (bootstrapOptions.databaseOpenMode == DatabaseOpenMode::OpenExisting) {
            restoreRuntimeState(bootstrapOptions);
            loadedRuntimeState = true;
        } else if (bootstrapOptions.databaseOpenMode == DatabaseOpenMode::CreateFromSeedIfMissing) {
            SqliteGameStateRepository runtimeRepository(bootstrapOptions.sqliteDbPath, GameStateRepositoryMode::ReadExisting);
            if (runtimeRepository.hasGameState()) {
                restoreRuntimeState(bootstrapOptions);
                loadedRuntimeState = true;
            }
        }
        break;
    default:
        throw std::invalid_argument("unsupported game bootstrap mode");
    }

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
            event.matchId,
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
    if (!loadedRuntimeState) {
        updateState();
        updateCurrentDateSaveMetadata();
        persistRuntimeState();
    }
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

    world.getTransferOfferService().expirePendingOffers(date);

    if (timePaused) {
        processBlockingEvent();
        return;
    }
   
    //gunluk mac kontrolu
    matchScheduler.update(world, date, eventsQueue);

    while (!eventsQueue.empty()) {
        const PlayMatchCommand command = eventsQueue.popNext();

        const LeagueId managedLeagueId = user.getManagedLeagueId();
        const TeamId managedTeamId = user.getManagedTeamId();
        const bool hasManagedTeam = managedLeagueId != 0 && managedTeamId != 0;
        const bool isManagedTeamMatch = hasManagedTeam && command.leagueId == managedLeagueId && (command.homeId == managedTeamId || command.awayId == managedTeamId);

        if (isManagedTeamMatch) {
            LeagueContext* context = world.findLeagueContext(command.leagueId);
            if (!context) {
                throw std::logic_error("managed pre-match command references unknown league context");
            }

            League& league = context->getLeague();
            const Team* homeTeam = league.findTeamById(command.homeId);
            const Team* awayTeam = league.findTeamById(command.awayId);
            if (!homeTeam || !awayTeam) {
                throw std::logic_error("managed pre-match command references unknown teams");
            }

            TeamSelectionService selectionService;
            TeamSheet homeSheet = selectionService.buildTeamSheet(*homeTeam);
            TeamSheet awaySheet = selectionService.buildTeamSheet(*awayTeam);
            validateTeamSheetForTeam(homeSheet, *homeTeam);
            validateTeamSheetForTeam(awaySheet, *awayTeam);

            pendingPreMatchCommand = command;
            pendingPreMatchHomeSheet = homeSheet;
            pendingPreMatchAwaySheet = awaySheet;

            interactionManager.enqueue(std::make_unique<PreMatchInteraction>(
                command.matchId,
                command.leagueId,
                command.date,
                command.homeId,
                command.awayId,
                command.matchweek,
                std::move(homeSheet),
                std::move(awaySheet)));
            refreshTimePauseState();
            persistRuntimeState();
            return;
        }

        LeagueContext* context = world.findLeagueContext(command.leagueId);
        if (!context) {
            throw std::logic_error("play command references unknown league context");
        }
        context->getPlayMatchCommandHandler().handle(context->getLeague(), command);
        persistRuntimeState();

        refreshTimePauseState();
        if (timePaused) {
            persistRuntimeState();
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
    bool didAdvanceDay = false;
    if (!dateWasReset) {
        date.advanceDay();
        didAdvanceDay = true;
    }
    else {
        dateWasReset = false;
    }

    if (didAdvanceDay) {
        PlayerConditionService conditionService;
        world.forEachLeagueContext([&conditionService](LeagueContext& context) {
            conditionService.applyDailyRecovery(context.getLeague());
            });
        updateCurrentDateSaveMetadata();
        persistRuntimeState();
    }
    world.getTransferOfferService().expirePendingOffers(date);//gecici cozum 2. kere expire kontrolu
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

PlayerId Game::pickNextDebugOfferCandidatePlayerId(const Team& sellerTeam) {
    const auto& sellerPlayers = sellerTeam.getPlayers();
    if (sellerPlayers.empty()) {
        return 0;
    }

    const std::size_t playerCount = sellerPlayers.size();
    const std::size_t startIndex = debugOfferPlayerCursor % playerCount;

    for (std::size_t offset = 0; offset < playerCount; ++offset) {
        const std::size_t index = (startIndex + offset) % playerCount;
        if (!sellerPlayers[index]) {
            continue;
        }
        const PlayerId playerId = sellerPlayers[index]->getId();
        if (playerId == 0) {
            continue;
        }

        debugOfferPlayerCursor = (index + 1) % playerCount;
        return playerId;
    }

    return 0;
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
    if (hasActiveBlockingInteraction()) {
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

    const int currentYear = date.getYear();
    const int currentMonth = static_cast<int>(date.getMonth());
    if (lastDebugOfferYear == currentYear && lastDebugOfferMonth == currentMonth) {
        return;
    }

    for (const auto& [candidateTeamId, candidateTeam] : league.getTeams()) {
        if (candidateTeamId == managedTeamId) {
            continue;
        }
        if (!candidateTeam) {
            continue;
        }

        // Temporary debug funding guard so that generated offers are actually testable via Accept.
        constexpr Money debugFundingBoost = 5000000;
        if (!candidateTeam->canAffordTransfer(1) || !candidateTeam->canAffordWage(1000)) {
            candidateTeam->earn(debugFundingBoost);
            candidateTeam->setBudgets();
        }

        if (!candidateTeam->canAffordTransfer(1) || !candidateTeam->canAffordWage(1000)) {
            continue;
        }

        Team* buyerTeam = candidateTeam.get();
        if (!buyerTeam) {
            return;
        }

        const PlayerId selectedPlayerId = pickNextDebugOfferCandidatePlayerId(*sellerTeam);
        if (selectedPlayerId == 0) {
            continue;
        }

        if (world.getTransferOfferService().hasPendingOfferForSellerPlayer(managedLeagueId, managedTeamId, selectedPlayerId)) {
            continue;
        }

        constexpr Money weeklyRuntimeOfferFee = 1;
        const OfferId createdOfferId = world.getTransferOfferService().createOffer(
            date,
            managedLeagueId,
            managedTeamId,
            managedLeagueId,
            buyerTeam->getId(),
            selectedPlayerId,
            weeklyRuntimeOfferFee);

        if (createdOfferId == 0) {
            continue;
        }

        lastDebugOfferYear = currentYear;
        lastDebugOfferMonth = currentMonth;
        return;
    }
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
    fixtureGenerator.generateSeasonFixture(league, seasonPlan, rules, [this]() { return world.allocateMatchId(); });
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

const PreMatchInteraction* Game::getActivePreMatchInteraction() const {
    return dynamic_cast<const PreMatchInteraction*>(interactionManager.getActiveInteraction());
}

bool Game::playPendingPreMatch() {
    const PreMatchInteraction* interaction = getActivePreMatchInteraction();
    if (!interaction || !pendingPreMatchCommand.has_value() || !pendingPreMatchHomeSheet.has_value() || !pendingPreMatchAwaySheet.has_value()) {
        return false;
    }

    const PlayMatchCommand command = *pendingPreMatchCommand;
    LeagueContext* context = world.findLeagueContext(command.leagueId);
    if (!context) {
        throw std::logic_error("pending pre-match command references unknown league context");
    }

    context->getPlayMatchCommandHandler().handle(
        context->getLeague(),
        command,
        *pendingPreMatchHomeSheet,
        *pendingPreMatchAwaySheet);
    pendingPreMatchCommand.reset();
    pendingPreMatchHomeSheet.reset();
    pendingPreMatchAwaySheet.reset();
    interactionManager.resolveActiveInteraction();
    refreshTimePauseState();
    updateCurrentDateSaveMetadata();
    persistRuntimeState();
    return true;
}

bool Game::replacePendingPreMatchTeamSheetForTeam(TeamId teamId, const TeamSheet& teamSheet) {
    if (teamId == 0 || teamSheet.teamId != teamId) {
        return false;
    }
    if (!pendingPreMatchCommand.has_value()
        || !pendingPreMatchHomeSheet.has_value()
        || !pendingPreMatchAwaySheet.has_value()) {
        return false;
    }

    PreMatchInteraction* interaction = dynamic_cast<PreMatchInteraction*>(interactionManager.getActiveInteraction());
    if (!interaction) {
        return false;
    }

    const PlayMatchCommand& command = *pendingPreMatchCommand;
    if (teamId != command.homeId && teamId != command.awayId) {
        return false;
    }

    if (!interaction->replaceTeamSheetForTeam(teamId, teamSheet)) {
        return false;
    }

    if (teamId == command.homeId) {
        pendingPreMatchHomeSheet = teamSheet;
    } else {
        pendingPreMatchAwaySheet = teamSheet;
    }

    return true;
}

bool Game::replaceActivePreMatchDisplayTeamSheetForTeam(TeamId teamId, const TeamSheet& teamSheet) {
    if (teamId == 0 || teamSheet.teamId != teamId) {
        return false;
    }

    PreMatchInteraction* interaction = dynamic_cast<PreMatchInteraction*>(interactionManager.getActiveInteraction());
    if (!interaction) {
        return false;
    }

    return interaction->replaceTeamSheetForTeam(teamId, teamSheet);
}

bool Game::resolveActiveInteraction() {
    if (getActivePreMatchInteraction()) {
        refreshTimePauseState();
        return false;
    }
    const bool resolved = interactionManager.resolveActiveInteraction();
    if (resolved) {
        userPaused = true;
    }
    refreshTimePauseState();
    return resolved;
}

bool Game::acceptTransferOffer(OfferId offerId) {
    if (offerId == 0) {
        return false;
    }

    const LeagueId managedLeagueId = user.getManagedLeagueId();
    const TeamId managedTeamId = user.getManagedTeamId();
    if (managedLeagueId == 0 || managedTeamId == 0) {
        return false;
    }

    const TransferOffer* offer = world.getTransferOfferService().findOfferById(offerId);
    if (!offer || offer->status != TransferOfferStatus::Pending) {
        return false;
    }

    if (offer->sellerLeagueId != managedLeagueId || offer->sellerTeamId != managedTeamId) {
        return false;
    }

    if (!world.getTransferOfferService().acceptOffer(offerId, date)) {
        return false;
    }

    const TransferOfferDecisionInteraction* interaction = getActiveTransferOfferDecisionInteraction();
    if (interaction && interaction->getOfferId() == offerId) {
        interactionManager.resolveActiveInteraction();
    }

    userPaused = true;
    refreshTimePauseState();
    return true;
}

bool Game::rejectTransferOffer(OfferId offerId) {
    if (offerId == 0) {
        return false;
    }

    const LeagueId managedLeagueId = user.getManagedLeagueId();
    const TeamId managedTeamId = user.getManagedTeamId();
    if (managedLeagueId == 0 || managedTeamId == 0) {
        return false;
    }

    const TransferOffer* offer = world.getTransferOfferService().findOfferById(offerId);
    if (!offer || offer->status != TransferOfferStatus::Pending) {
        return false;
    }

    if (offer->sellerLeagueId != managedLeagueId || offer->sellerTeamId != managedTeamId) {
        return false;
    }

    if (!world.getTransferOfferService().rejectOffer(offerId)) {
        return false;
    }

    const TransferOfferDecisionInteraction* interaction = getActiveTransferOfferDecisionInteraction();
    if (interaction && interaction->getOfferId() == offerId) {
        interactionManager.resolveActiveInteraction();
    }
    userPaused = true;
    refreshTimePauseState();
    return true;
}

std::vector<const TransferOffer*> Game::getPendingTransferOffersForManagedTeam() const {
    const LeagueId managedLeagueId = user.getManagedLeagueId();
    const TeamId managedTeamId = user.getManagedTeamId();
    if (managedLeagueId == 0 || managedTeamId == 0) {
        return {};
    }

    return world.getTransferOfferService().getPendingOffersForSellerTeam(managedLeagueId, managedTeamId);
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
    updateManagedClubSaveMetadata();
}

void Game::setSaveManagerName(const std::string& managerName) {
    if (!saveMetadataEnabled) {
        saveMetadata.managerName = managerName.empty() ? "Manager" : managerName;
        return;
    }

    SqliteSaveMetadataRepository repository(saveMetadataDbPath);
    repository.updateManagerName(managerName);
    repository.touchUpdatedAt();
    saveMetadata = repository.load();
}

LeagueId Game::getManagedLeagueId() const {
    return user.getManagedLeagueId();
}

TeamId Game::getManagedTeamId() const {
    return user.getManagedTeamId();
}

SaveMetadata Game::getSaveMetadata() const {
    return saveMetadata;
}

GameState Game::getState() const {
    return state;
}


//debug
const MatchScheduler& Game::getMatchScheduler() const {
    return matchScheduler;
}
