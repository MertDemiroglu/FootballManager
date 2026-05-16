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
    Date initialGameDate() {
        return Date(2025, Month::July, 1);
    }

    std::string dateToIsoString(const Date& date) {
        std::ostringstream output;
        output << std::setw(4) << std::setfill('0') << date.getYear() << "-"
            << std::setw(2) << std::setfill('0') << static_cast<int>(date.getMonth()) << "-"
            << std::setw(2) << std::setfill('0') << date.getDay();
        return output.str();
    }

    bool sameDate(const Date& lhs, const Date& rhs) {
        return lhs.getYear() == rhs.getYear()
            && lhs.getMonth() == rhs.getMonth()
            && lhs.getDay() == rhs.getDay();
    }

    bool dateIsBefore(const Date& lhs, const Date& rhs) {
        return lhs < rhs;
    }

    bool dateIsOnOrAfter(const Date& lhs, const Date& rhs) {
        return !dateIsBefore(lhs, rhs);
    }

    bool sqliteDatabaseFileExists(const std::string& dbPath) {
        std::error_code error;
        const bool exists = std::filesystem::exists(dbPath, error);
        if (error) {
            throw std::invalid_argument("failed to inspect sqlite db path '" + dbPath + "': " + error.message());
        }
        return exists;
    }

    struct PlayerLocation {
        LeagueId leagueId = 0;
        TeamId teamId = 0;
        League* league = nullptr;
        Team* team = nullptr;
        Footballer* player = nullptr;
    };

    enum class TeamSheetReconcilePolicy {
        PreserveManualGaps,
        AutoFill
    };

    std::optional<PlayerLocation> findPlayerLocationById(World& world, PlayerId playerId) {
        if (playerId == 0) {
            return std::nullopt;
        }

        std::optional<PlayerLocation> location;
        world.forEachLeagueContext([&](LeagueContext& context) {
            if (location.has_value()) {
                return;
            }
            League& league = context.getLeague();
            for (const auto& [teamId, team] : league.getTeams()) {
                if (!team) {
                    continue;
                }
                Footballer* player = team->findPlayerById(playerId);
                if (!player) {
                    continue;
                }
                location = PlayerLocation{ league.getId(), teamId, &league, team.get(), player };
                return;
            }
        });
        return location;
    }

    void restoreTeamFinances(World& world, const std::vector<PersistedTeamFinanceState>& teamFinances) {
        if (teamFinances.empty()) {
            throw std::runtime_error("save slot is missing runtime team finance state; clear incompatible development saves or start a new game");
        }

        for (const PersistedTeamFinanceState& state : teamFinances) {
            LeagueContext* context = world.findLeagueContext(state.leagueId);
            if (!context) {
                throw std::runtime_error("runtime team finance references unknown league");
            }
            Team* team = context->getLeague().findTeamById(state.teamId);
            if (!team) {
                throw std::runtime_error("runtime team finance references unknown team");
            }
            team->setBudgetSnapshot(state.totalBudget, state.transferBudget, state.wageBudget);
        }
    }

    void restorePlayerRosterState(World& world, const std::vector<PersistedPlayerRosterState>& rosterStates) {
        if (rosterStates.empty()) {
            throw std::runtime_error("save slot is missing runtime player roster state; clear incompatible development saves or start a new game");
        }

        std::unordered_set<PlayerId> restoredPlayerIds;
        for (const PersistedPlayerRosterState& state : rosterStates) {
            if (!restoredPlayerIds.insert(state.playerId).second) {
                throw std::runtime_error("runtime player roster state has duplicate player id");
            }

            LeagueContext* targetContext = world.findLeagueContext(state.leagueId);
            if (!targetContext) {
                throw std::runtime_error("runtime player roster state references unknown league");
            }
            Team* targetTeam = targetContext->getLeague().findTeamById(state.teamId);
            if (!targetTeam) {
                throw std::runtime_error("runtime player roster state references unknown team");
            }

            std::optional<PlayerLocation> currentLocation = findPlayerLocationById(world, state.playerId);
            if (!currentLocation.has_value() || !currentLocation->team) {
                throw std::runtime_error("runtime player roster state references unknown player");
            }

            Footballer* restoredPlayer = nullptr;
            if (currentLocation->leagueId == state.leagueId && currentLocation->teamId == state.teamId) {
                restoredPlayer = currentLocation->player;
            }
            else {
                if (targetTeam->findPlayerById(state.playerId)) {
                    throw std::runtime_error("runtime player roster restore would duplicate a player");
                }

                std::unique_ptr<Footballer> movingPlayer = currentLocation->team->releasePlayer(state.playerId);
                if (!movingPlayer) {
                    throw std::runtime_error("runtime player roster restore failed to release player from source team");
                }
                targetTeam->addPlayer(std::move(movingPlayer));
                restoredPlayer = targetTeam->findPlayerById(state.playerId);
            }

            if (!restoredPlayer) {
                throw std::runtime_error("runtime player roster restore failed to locate restored player");
            }
            restoredPlayer->setTeam(state.teamId);

            if (state.wage.has_value() != state.contractYears.has_value()) {
                throw std::runtime_error("runtime player roster state has partial contract snapshot");
            }
            if (state.wage.has_value() && state.contractYears.has_value()) {
                restoredPlayer->signContract(*state.wage, *state.contractYears);
            }
            if (state.currentSeasonYear >= 0
                && restoredPlayer->getCurrentSeasonStats().seasonYear != state.currentSeasonYear) {
                restoredPlayer->initializeSeasonStats(state.currentSeasonYear);
            }
        }
    }

    FormationId resolveTeamSheetFormationForReconcile(const TeamSheet& sheet, const Team& team) {
        if (isFormationSupported(sheet.formation)) {
            return sheet.formation;
        }
        const TacticalPreferences& preferences = team.getHeadCoach().getTacticalPreferences();
        if (isFormationSupported(preferences.preferredFormation)) {
            return preferences.preferredFormation;
        }
        return getSupportedFormationIds().front();
    }

    TeamSheet sanitizeTeamSheetForTeamPreservingGaps(const TeamSheet& sheet, const Team& team) {
        std::unordered_set<PlayerId> rosterPlayerIds;
        for (const auto& player : team.getPlayers()) {
            if (!player || player->getId() == 0) {
                continue;
            }
            rosterPlayerIds.insert(player->getId());
        }

        TeamSheet sanitizedSheet;
        sanitizedSheet.teamId = team.getId();
        sanitizedSheet.coachId = team.getHeadCoach().getId();
        sanitizedSheet.formation = resolveTeamSheetFormationForReconcile(sheet, team);
        sanitizedSheet.tacticalSetup = sheet.tacticalSetup;

        std::unordered_set<PlayerId> usedPlayerIds;
        const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(sanitizedSheet.formation);
        for (const TeamSheetSlotAssignment& assignment : sheet.startingAssignments) {
            if (assignment.slotIndex >= slotTemplate.size()) {
                continue;
            }
            if (slotTemplate[assignment.slotIndex] != assignment.slotRole) {
                continue;
            }
            if (assignment.playerId == 0 || rosterPlayerIds.find(assignment.playerId) == rosterPlayerIds.end()) {
                continue;
            }
            if (!usedPlayerIds.insert(assignment.playerId).second) {
                continue;
            }

            sanitizedSheet.startingAssignments.push_back(assignment);
            sanitizedSheet.startingPlayerIds.push_back(assignment.playerId);
        }

        for (PlayerId playerId : sheet.substitutePlayerIds) {
            if (playerId == 0 || rosterPlayerIds.find(playerId) == rosterPlayerIds.end()) {
                continue;
            }
            if (!usedPlayerIds.insert(playerId).second) {
                continue;
            }
            if (sanitizedSheet.substitutePlayerIds.size() >= kMaxSubstituteCount) {
                break;
            }
            sanitizedSheet.substitutePlayerIds.push_back(playerId);
        }

        validateTeamSheetForTeam(sanitizedSheet, team);
        return sanitizedSheet;
    }

    TeamSheet reconcileTeamSheetForTeamAutoFill(const TeamSheet& sheet, const Team& team) {
        TeamSheet reconciledSheet = sanitizeTeamSheetForTeamPreservingGaps(sheet, team);

        TeamSelectionService selectionService;
        TeamSheet generatedSheet = selectionService.buildTeamSheet(team, reconciledSheet.formation);
        generatedSheet.tacticalSetup = reconciledSheet.tacticalSetup;
        generatedSheet.coachId = team.getHeadCoach().getId();

        std::unordered_set<PlayerId> rosterPlayerIds;
        std::vector<PlayerId> rosterOrder;
        for (const auto& player : team.getPlayers()) {
            if (!player || player->getId() == 0) {
                continue;
            }
            rosterPlayerIds.insert(player->getId());
            rosterOrder.push_back(player->getId());
        }

        std::unordered_set<PlayerId> usedPlayerIds;
        for (const TeamSheetSlotAssignment& assignment : reconciledSheet.startingAssignments) {
            usedPlayerIds.insert(assignment.playerId);
        }
        for (PlayerId playerId : reconciledSheet.substitutePlayerIds) {
            usedPlayerIds.insert(playerId);
        }

        const auto slotFilled = [&](std::size_t slotIndex) {
            for (const TeamSheetSlotAssignment& assignment : reconciledSheet.startingAssignments) {
                if (assignment.slotIndex == slotIndex) {
                    return true;
                }
            }
            return false;
        };

        const auto appendStarter = [&](const TeamSheetSlotAssignment& assignment) {
            if (assignment.playerId == 0 || rosterPlayerIds.find(assignment.playerId) == rosterPlayerIds.end()) {
                return false;
            }
            if (slotFilled(assignment.slotIndex)) {
                return false;
            }
            if (!usedPlayerIds.insert(assignment.playerId).second) {
                return false;
            }
            reconciledSheet.startingAssignments.push_back(assignment);
            reconciledSheet.startingPlayerIds.push_back(assignment.playerId);
            return true;
        };

        for (const TeamSheetSlotAssignment& generatedAssignment : generatedSheet.startingAssignments) {
            (void)appendStarter(generatedAssignment);
        }

        for (const PlayerId playerId : rosterOrder) {
            if (reconciledSheet.startingAssignments.size() >= generatedSheet.startingAssignments.size()) {
                break;
            }
            if (usedPlayerIds.find(playerId) != usedPlayerIds.end()) {
                continue;
            }
            for (const TeamSheetSlotAssignment& generatedAssignment : generatedSheet.startingAssignments) {
                if (!slotFilled(generatedAssignment.slotIndex)) {
                    (void)appendStarter(TeamSheetSlotAssignment{ generatedAssignment.slotIndex, generatedAssignment.slotRole, playerId });
                    break;
                }
            }
        }

        const auto appendSubstitute = [&](PlayerId playerId) {
            if (playerId == 0 || rosterPlayerIds.find(playerId) == rosterPlayerIds.end()) {
                return false;
            }
            if (usedPlayerIds.find(playerId) != usedPlayerIds.end()) {
                return false;
            }
            if (reconciledSheet.substitutePlayerIds.size() >= kMaxSubstituteCount) {
                return false;
            }
            usedPlayerIds.insert(playerId);
            reconciledSheet.substitutePlayerIds.push_back(playerId);
            return true;
        };

        for (PlayerId playerId : generatedSheet.substitutePlayerIds) {
            (void)appendSubstitute(playerId);
        }
        for (PlayerId playerId : rosterOrder) {
            if (reconciledSheet.substitutePlayerIds.size() >= kMaxSubstituteCount) {
                break;
            }
            (void)appendSubstitute(playerId);
        }

        validateTeamSheetForTeam(reconciledSheet, team);
        return reconciledSheet;
    }

    TeamSheet reconcileTeamSheetForTeam(
        const TeamSheet& sheet,
        const Team& team,
        TeamSheetReconcilePolicy policy) {
        if (policy == TeamSheetReconcilePolicy::PreserveManualGaps) {
            return sanitizeTeamSheetForTeamPreservingGaps(sheet, team);
        }
        return reconcileTeamSheetForTeamAutoFill(sheet, team);
    }

    PersistedTransferOfferState toPersistedTransferOfferState(const TransferOffer& offer) {
        PersistedTransferOfferState state;
        state.offerId = offer.id;
        state.createdAt = offer.createdAt;
        state.lastValidDate = offer.lastValidDate;
        state.expiryPolicy = offer.expiryPolicy;
        state.sellerLeagueId = offer.sellerLeagueId;
        state.sellerTeamId = offer.sellerTeamId;
        state.buyerLeagueId = offer.buyerLeagueId;
        state.buyerTeamId = offer.buyerTeamId;
        state.playerId = offer.playerId;
        state.fee = offer.fee;
        state.status = offer.status;
        state.resolution = offer.resolution;
        return state;
    }

    TransferOffer toTransferOffer(const PersistedTransferOfferState& state) {
        TransferOffer offer;
        offer.id = state.offerId;
        offer.createdAt = state.createdAt;
        offer.lastValidDate = state.lastValidDate;
        offer.expiryPolicy = state.expiryPolicy;
        offer.sellerLeagueId = state.sellerLeagueId;
        offer.sellerTeamId = state.sellerTeamId;
        offer.buyerLeagueId = state.buyerLeagueId;
        offer.buyerTeamId = state.buyerTeamId;
        offer.playerId = state.playerId;
        offer.fee = state.fee;
        offer.status = state.status;
        offer.resolution = state.resolution;
        return offer;
    }

    void requireSqliteSeedAssets(const GameBootstrapOptions& bootstrapOptions, const std::string& context) {
        if (bootstrapOptions.sqliteSchemaPath.empty()) {
            throw std::invalid_argument("sqlite schema path cannot be empty " + context);
        }
        if (bootstrapOptions.sqliteSeedPath.empty()) {
            throw std::invalid_argument("sqlite seed path cannot be empty " + context);
        }
    }

    void bootstrapWorldFromSqlite(World& world, const GameBootstrapOptions& bootstrapOptions, int initialSeasonYear) {
        if (bootstrapOptions.sqliteDbPath.empty()) {
            throw std::invalid_argument("sqlite db path cannot be empty");
        }

        switch (bootstrapOptions.databaseOpenMode) {
        case DatabaseOpenMode::OpenExisting:
            if (!sqliteDatabaseFileExists(bootstrapOptions.sqliteDbPath)) {
                throw std::invalid_argument("sqlite db path does not exist for OpenExisting mode: " + bootstrapOptions.sqliteDbPath);
            }
            WorldBootstrapService::loadIntoWorldFromSqlite(world, bootstrapOptions.sqliteDbPath, initialSeasonYear);
            return;
        case DatabaseOpenMode::CreateFromSeedIfMissing:
            if (sqliteDatabaseFileExists(bootstrapOptions.sqliteDbPath)) {
                WorldBootstrapService::loadIntoWorldFromSqlite(world, bootstrapOptions.sqliteDbPath, initialSeasonYear);
                return;
            }
            requireSqliteSeedAssets(bootstrapOptions, "when creating missing sqlite database from seed");
            WorldBootstrapService::initializeAndLoadIntoWorldFromSqlite(
                world,
                bootstrapOptions.sqliteDbPath,
                bootstrapOptions.sqliteSchemaPath,
                bootstrapOptions.sqliteSeedPath,
                initialSeasonYear);
            return;
        case DatabaseOpenMode::ResetFromSeed:
            requireSqliteSeedAssets(bootstrapOptions, "when resetting sqlite database from seed");
            WorldBootstrapService::resetAndLoadIntoWorldFromSqlite(
                world,
                bootstrapOptions.sqliteDbPath,
                bootstrapOptions.sqliteSchemaPath,
                bootstrapOptions.sqliteSeedPath,
                initialSeasonYear);
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
        defaultMetadata.schemaVersion = 6;
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
    const std::vector<PersistedTeamSheetState> teamSheetStates = repository.loadTeamSheetStates();
    const std::vector<PersistedPlayerRuntimeState> playerStates = repository.loadPlayerRuntimeStates();
    const std::vector<PersistedTeamFinanceState> teamFinanceStates = repository.loadTeamFinanceStates();
    const std::vector<PersistedPlayerRosterState> playerRosterStates = repository.loadPlayerRosterStates();
    const std::vector<PersistedTransferOfferState> transferOfferStates = repository.loadTransferOfferStates();

    if (leagueStates.empty()) {
        throw std::runtime_error("save slot is missing league runtime state");
    }
    if (fixtures.empty()) {
        throw std::runtime_error("save slot is missing persisted fixtures");
    }
    if (playerStates.empty()) {
        throw std::runtime_error("save slot is missing player runtime state");
    }
    if (teamFinanceStates.empty()) {
        throw std::runtime_error("save slot is missing runtime team finance state; clear incompatible development saves or start a new game");
    }
    if (playerRosterStates.empty()) {
        throw std::runtime_error("save slot is missing runtime player roster state; clear incompatible development saves or start a new game");
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
        // event_enqueued is intentionally not restored yet because active
        // blocking interactions are not persisted in this phase. Reloading a
        // pre-match pause should allow the scheduler to enqueue that match
        // again instead of leaving it permanently skipped. Revisit this when
        // active interaction persistence is added.
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

    restoreTeamFinances(world, teamFinanceStates);
    restorePlayerRosterState(world, playerRosterStates);

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

    for (PersistedTeamSheetState state : teamSheetStates) {
        LeagueContext* context = world.findLeagueContext(state.leagueId);
        if (!context) {
            throw std::runtime_error("runtime team sheet references unknown league");
        }

        Team* team = context->getLeague().findTeamById(state.teamSheet.teamId);
        if (!team) {
            throw std::runtime_error("runtime team sheet references unknown team");
        }

        const bool isManagedTeamSheet =
            state.leagueId == user.getManagedLeagueId()
            && state.teamSheet.teamId == user.getManagedTeamId();
        const TeamSheetReconcilePolicy policy = isManagedTeamSheet
            ? TeamSheetReconcilePolicy::PreserveManualGaps
            : TeamSheetReconcilePolicy::AutoFill;
        TeamSheet reconciledSheet = reconcileTeamSheetForTeam(state.teamSheet, *team, policy);
        // TODO: Managed-team incomplete sheets should become a "lineup requires attention"
        // interaction before kickoff once active interactions persist.
        team->setSelectedTeamSheet(std::move(reconciledSheet));
    }

    std::vector<TransferOffer> transferOffers;
    transferOffers.reserve(transferOfferStates.size());
    for (const PersistedTransferOfferState& state : transferOfferStates) {
        transferOffers.push_back(toTransferOffer(state));
    }
    // Restoring resolved transfer offers only restores offer state/history.
    // Accepted transfer roster/budget effects are restored from runtime roster/finance tables.
    world.getTransferOfferService().restoreOffers(std::move(transferOffers));

    SqliteSaveMetadataRepository metadataRepository(saveMetadataDbPath);
    metadataRepository.updateCurrentDate(dateToIsoString(date));
    saveMetadata = metadataRepository.load();
    validateRuntimeDateConsistency("restore runtime state");
}

void Game::persistRuntimeState() {
    if (!saveMetadataEnabled) {
        return;
    }

    validateRuntimeDateConsistency("persist runtime state");

    std::vector<PersistedLeagueRuntimeState> leagueStates;
    std::vector<PersistedFixtureState> fixtures;
    std::vector<MatchReport> reports;
    std::vector<PersistedTeamSheetState> teamSheetStates;
    std::vector<PersistedPlayerRuntimeState> playerStates;
    std::vector<PersistedTeamFinanceState> teamFinances;
    std::vector<PersistedPlayerRosterState> playerRosterStates;
    std::vector<PersistedTransferOfferState> transferOffers;

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
                    // event_enqueued is a transient scheduler guard, not an
                    // authoritative save value until active blocking
                    // interactions are persisted.
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
            if (const TeamSheet* selectedTeamSheet = team->getSelectedTeamSheet()) {
                teamSheetStates.push_back(PersistedTeamSheetState{ league.getId(), *selectedTeamSheet });
            }
            teamFinances.push_back(PersistedTeamFinanceState{
                league.getId(),
                team->getId(),
                team->getTotalBudget(),
                team->getTransferBudget(),
                team->getWageBudget()
            });
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
                PersistedPlayerRosterState rosterState;
                rosterState.playerId = player->getId();
                rosterState.leagueId = league.getId();
                rosterState.teamId = team->getId();
                if (const Contract* contract = player->getContract()) {
                    rosterState.wage = contract->getWage();
                    rosterState.contractYears = contract->getYearsRemaining();
                }
                rosterState.currentSeasonYear = player->getCurrentSeasonStats().seasonYear;
                playerRosterStates.push_back(rosterState);
            }
        }
    });

    for (const TransferOffer& offer : world.getTransferOfferService().exportOffers()) {
        transferOffers.push_back(toPersistedTransferOfferState(offer));
    }

    SqliteGameStateRepository repository(saveMetadataDbPath);
    repository.saveRuntimeState(
        date,
        static_cast<int>(state),
        leagueStates,
        fixtures,
        reports,
        teamSheetStates,
        playerStates,
        teamFinances,
        playerRosterStates,
        transferOffers);
}

void Game::loadSaveSettings() {
    if (!saveMetadataEnabled) {
        saveScheduler.setAutoSaveFrequency(AutoSaveFrequency::Weekly);
        saveScheduler.setLastAutoSaveDate(std::nullopt);
        return;
    }

    SqliteGameStateRepository repository(saveMetadataDbPath);
    const PersistedSaveSettings settings = repository.loadSaveSettings();
    saveScheduler.setAutoSaveFrequency(settings.autoSaveFrequency);
    saveScheduler.setLastAutoSaveDate(settings.lastAutoSaveDate);
}

void Game::persistSaveSettings() const {
    if (!saveMetadataEnabled) {
        return;
    }

    SqliteGameStateRepository repository(saveMetadataDbPath);
    repository.saveSaveSettings(PersistedSaveSettings{
        saveScheduler.getAutoSaveFrequency(),
        saveScheduler.getLastAutoSaveDate()
    });
}

void Game::requestRuntimeSave(SaveReason reason) {
    saveScheduler.requestSave(reason);
}

void Game::flushPendingRuntimeSaveIfNeeded() {
    if (!saveScheduler.hasPendingSave()) {
        return;
    }

    flushRuntimeSave(saveScheduler.getPendingReason().value_or(SaveReason::Manual));
}

void Game::flushRuntimeSave(SaveReason reason) {
    (void)reason;
    updateCurrentDateSaveMetadata();
    persistRuntimeState();
    saveScheduler.markSaved(date);
    persistSaveSettings();
}

void Game::checkScheduledAutoSaveCheckpoint() {
    if (saveScheduler.shouldScheduledAutosave(date)) {
        requestRuntimeSave(SaveReason::ScheduledAutoSave);
    }
    flushPendingRuntimeSaveIfNeeded();
}

void Game::validateRuntimeDateConsistency(const char* context) const {
    world.forEachLeagueContext([&](const LeagueContext& leagueContext) {
        const League& league = leagueContext.getLeague();
        const LeagueRules& rules = leagueContext.getRules();
        const SeasonPlan& seasonPlan = leagueContext.getSeasonPlan();
        const int seasonYear = league.getCurrentSeasonYear();

        if (seasonYear < 0) {
            throw std::logic_error(std::string(context) + ": league runtime season year is not initialized");
        }

        const Date preseasonStart = rules.preseasonStart(seasonYear);
        const Date nextPreseasonStart = rules.nextPreseasonStart(seasonYear);
        if (dateIsBefore(date, preseasonStart)
            || (dateIsOnOrAfter(date, nextPreseasonStart) && !sameDate(date, nextPreseasonStart))) {
            throw std::logic_error(std::string(context) + ": game date is outside the league runtime season window");
        }

        if (league.isSeasonFixtureGenerated() && league.debugTotalFixtureMatches() == 0) {
            throw std::logic_error(std::string(context) + ": fixture generation flag is set but no fixtures exist");
        }

        if (!league.isSeasonFixtureGenerated()) {
            return;
        }

        if (!sameDate(seasonPlan.getPreseasonStart(), preseasonStart)) {
            throw std::logic_error(std::string(context) + ": season plan is not aligned with league runtime season year");
        }

        for (const auto& [fixtureDate, matches] : league.getFixture()) {
            if (fixtureDate.getYear() < seasonYear || fixtureDate.getYear() > seasonYear + 1) {
                throw std::logic_error(std::string(context) + ": fixture date is outside the league runtime season year");
            }
            for (const FixtureMatch& match : matches) {
                if (match.matchweek <= 0) {
                    throw std::logic_error(std::string(context) + ": fixture has invalid matchweek");
                }
                if (!league.hasTeam(match.homeId) || !league.hasTeam(match.awayId)) {
                    throw std::logic_error(std::string(context) + ": fixture references unknown team");
                }
            }
        }
    });
}

TeamSheet Game::createTeamSheetForTeam(const Team& team) const {
    TeamSelectionService selectionService;
    const TacticalPreferences& preferences = team.getHeadCoach().getTacticalPreferences();
    const FormationId formation = isFormationSupported(preferences.preferredFormation)
        ? preferences.preferredFormation
        : getSupportedFormationIds().front();

    TeamSheet teamSheet = selectionService.buildTeamSheet(team, formation);
    teamSheet.tacticalSetup.mentality = preferences.preferredMentality;
    teamSheet.tacticalSetup.tempo = preferences.preferredTempo;
    validateTeamSheetForTeam(teamSheet, team);
    return teamSheet;
}

TeamSheet& Game::ensureTeamSheetForTeam(LeagueId leagueId, TeamId teamId) {
    LeagueContext* context = world.findLeagueContext(leagueId);
    if (!context) {
        throw std::runtime_error("cannot ensure selected team sheet for unknown league");
    }

    Team* team = context->getLeague().findTeamById(teamId);
    if (!team) {
        throw std::runtime_error("cannot ensure selected team sheet for unknown team");
    }

    if (TeamSheet* selectedTeamSheet = team->getSelectedTeamSheet()) {
        selectedTeamSheet->teamId = teamId;
        selectedTeamSheet->coachId = team->getHeadCoach().getId();
        validateTeamSheetForTeam(*selectedTeamSheet, *team);
        return *selectedTeamSheet;
    }

    team->setSelectedTeamSheet(createTeamSheetForTeam(*team));
    TeamSheet* selectedTeamSheet = team->getSelectedTeamSheet();
    if (!selectedTeamSheet) {
        throw std::logic_error("failed to store selected team sheet on team");
    }
    return *selectedTeamSheet;
}

TeamSheet Game::resolveCompleteTeamSheetForTeam(LeagueId leagueId, TeamId teamId) {
    LeagueContext* context = world.findLeagueContext(leagueId);
    if (!context) {
        throw std::runtime_error("cannot resolve team sheet for unknown league");
    }

    Team* team = context->getLeague().findTeamById(teamId);
    if (!team) {
        throw std::runtime_error("cannot resolve team sheet for unknown team");
    }

    TeamSheet& selectedSheet = ensureTeamSheetForTeam(leagueId, teamId);
    const std::size_t expectedStarterCount = getFormationSlotTemplate(selectedSheet.formation).size();
    const bool hasCompleteStarters =
        selectedSheet.startingAssignments.size() == expectedStarterCount
        && selectedSheet.startingPlayerIds.size() == expectedStarterCount;
    if (hasCompleteStarters) {
        return selectedSheet;
    }

    const bool isManagedTeam =
        leagueId == user.getManagedLeagueId()
        && teamId == user.getManagedTeamId();
    if (isManagedTeam) {
        // TODO: Managed-team incomplete lineups should block kickoff with a
        // "lineup requires attention" interaction instead of falling through
        // to play-match validation.
        return selectedSheet;
    }

    TeamSelectionService selectionService;
    // TODO: AI matchday sheets should later reconcile availability, condition,
    // injuries, suspensions, transfers, and coach tactical profile.
    TeamSheet generatedSheet = selectionService.buildTeamSheet(*team, selectedSheet.formation);
    generatedSheet.tacticalSetup = selectedSheet.tacticalSetup;
    validateTeamSheetForTeam(generatedSheet, *team);
    team->setSelectedTeamSheet(generatedSheet);
    return *team->getSelectedTeamSheet();
}

void Game::reconcileSelectedTeamSheetForTeam(LeagueId leagueId, TeamId teamId, bool preserveManualGaps) {
    LeagueContext* context = world.findLeagueContext(leagueId);
    if (!context) {
        return;
    }

    Team* team = context->getLeague().findTeamById(teamId);
    if (!team) {
        return;
    }

    const TeamSheet* selectedTeamSheet = team->getSelectedTeamSheet();
    if (!selectedTeamSheet) {
        return;
    }

    const TeamSheetReconcilePolicy policy = preserveManualGaps
        ? TeamSheetReconcilePolicy::PreserveManualGaps
        : TeamSheetReconcilePolicy::AutoFill;
    TeamSheet reconciledSheet = reconcileTeamSheetForTeam(*selectedTeamSheet, *team, policy);
    team->setSelectedTeamSheet(std::move(reconciledSheet));
}

Game::~Game() = default;

Game::Game(const GameBootstrapOptions& bootstrapOptions)
    : date(initialGameDate()),
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
    bool loadedRuntimeState = false;

    switch (bootstrapOptions.mode) {
    case GameBootstrapMode::Sqlite:
        bootstrapWorldFromSqlite(world, bootstrapOptions, initialGameDate().getYear());
        ensureSaveMetadata(bootstrapOptions);
        loadSaveSettings();
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
    if (loadedRuntimeState && world.getTransferOfferService().expirePendingOffers(date) > 0) {
        requestRuntimeSave(SaveReason::TransferOfferResolved);
        flushPendingRuntimeSaveIfNeeded();
    }
    if (loadedRuntimeState && !saveScheduler.getLastAutoSaveDate().has_value()) {
        saveScheduler.markSaved(date);
        persistSaveSettings();
    }
    if (!loadedRuntimeState) {
        updateState();
        flushRuntimeSave(SaveReason::Manual);
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

    updateTransferWindow();
    if (world.getTransferOfferService().expirePendingOffers(date) > 0) {
        requestRuntimeSave(SaveReason::TransferOfferResolved);
    }

    if (timePaused) {
        processBlockingEvent();
        flushPendingRuntimeSaveIfNeeded();
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

            TeamSheet homeSheet = resolveCompleteTeamSheetForTeam(command.leagueId, command.homeId);
            TeamSheet awaySheet = resolveCompleteTeamSheetForTeam(command.leagueId, command.awayId);
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
            requestRuntimeSave(SaveReason::TeamSheetChanged);
            flushPendingRuntimeSaveIfNeeded();
            return;
        }

        LeagueContext* context = world.findLeagueContext(command.leagueId);
        if (!context) {
            throw std::logic_error("play command references unknown league context");
        }
        TeamSheet homeSheet = resolveCompleteTeamSheetForTeam(command.leagueId, command.homeId);
        TeamSheet awaySheet = resolveCompleteTeamSheetForTeam(command.leagueId, command.awayId);
        context->getPlayMatchCommandHandler().handle(context->getLeague(), command, homeSheet, awaySheet);
        requestRuntimeSave(SaveReason::MatchCompleted);

        refreshTimePauseState();
        if (timePaused) {
            flushPendingRuntimeSaveIfNeeded();
            return;
        }
    }

    handleSeasonalEvents();
    refreshTimePauseState();
    if (timePaused) {
        flushPendingRuntimeSaveIfNeeded();
        return;
    }

    updateTransferWindow();
    refreshTimePauseState();
    if (timePaused) {
        flushPendingRuntimeSaveIfNeeded();
        return;
    }

    if (date.isNewWeek()) {
        handleWeeklyEvents();
        refreshTimePauseState();
        if (timePaused) {
            flushPendingRuntimeSaveIfNeeded();
            return;
        }
    }

    if (date.isNewMonth()) { 
        handleMonthlyEvents();
        refreshTimePauseState();
        if (timePaused) {
            flushPendingRuntimeSaveIfNeeded();
            return;
        }
    }

    updateState();
    refreshTimePauseState();
    if (timePaused) {
        flushPendingRuntimeSaveIfNeeded();
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
    }
    updateTransferWindow();
    if (world.getTransferOfferService().expirePendingOffers(date) > 0) {
        requestRuntimeSave(SaveReason::TransferOfferResolved);
    }
    checkScheduledAutoSaveCheckpoint();
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
        requestRuntimeSave(SaveReason::TransferOfferResolved);
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
    requestRuntimeSave(SaveReason::SeasonRollover);
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
    requestRuntimeSave(SaveReason::MatchCompleted);
    flushPendingRuntimeSaveIfNeeded();
    return true;
}

std::optional<TeamSheet> Game::getSelectedTeamSheetForTeam(LeagueId leagueId, TeamId teamId) const {
    const LeagueContext* context = world.findLeagueContext(leagueId);
    if (!context) {
        return std::nullopt;
    }

    const Team* team = context->getLeague().findTeamById(teamId);
    if (!team) {
        return std::nullopt;
    }

    const TeamSheet* selectedTeamSheet = team->getSelectedTeamSheet();
    if (!selectedTeamSheet) {
        return std::nullopt;
    }

    return *selectedTeamSheet;
}

bool Game::updateSelectedTeamSheetForTeam(LeagueId leagueId, TeamId teamId, const TeamSheet& teamSheet) {
    if (leagueId == 0 || teamId == 0 || teamSheet.teamId != teamId) {
        return false;
    }

    LeagueContext* context = world.findLeagueContext(leagueId);
    if (!context) {
        return false;
    }

    Team* team = context->getLeague().findTeamById(teamId);
    if (!team) {
        return false;
    }

    TeamSheet selectedSheet = teamSheet;
    selectedSheet.coachId = team->getHeadCoach().getId();
    try {
        validateTeamSheetForTeam(selectedSheet, *team);
        team->setSelectedTeamSheet(std::move(selectedSheet));
        requestRuntimeSave(SaveReason::TeamSheetChanged);
    }
    catch (...) {
        return false;
    }

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

    (void)updateSelectedTeamSheetForTeam(command.leagueId, teamId, teamSheet);

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

    const bool accepted = world.getTransferOfferService().acceptOffer(offerId, date);
    if (!accepted) {
        requestRuntimeSave(SaveReason::TransferOfferResolved);
        flushPendingRuntimeSaveIfNeeded();
        return false;
    }

    const auto isManagedAffectedTeam = [&](LeagueId leagueId, TeamId teamId) {
        return leagueId == managedLeagueId && teamId == managedTeamId;
    };
    reconcileSelectedTeamSheetForTeam(
        offer->sellerLeagueId,
        offer->sellerTeamId,
        isManagedAffectedTeam(offer->sellerLeagueId, offer->sellerTeamId));
    reconcileSelectedTeamSheetForTeam(
        offer->buyerLeagueId,
        offer->buyerTeamId,
        isManagedAffectedTeam(offer->buyerLeagueId, offer->buyerTeamId));
    requestRuntimeSave(SaveReason::TransferAccepted);
    flushPendingRuntimeSaveIfNeeded();

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
    requestRuntimeSave(SaveReason::TransferOfferResolved);
    flushPendingRuntimeSaveIfNeeded();

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

void Game::flushSaveState() {
    flushRuntimeSave(saveScheduler.getPendingReason().value_or(SaveReason::Manual));
}

bool Game::manualSave() {
    if (!saveMetadataEnabled) {
        return false;
    }

    flushRuntimeSave(SaveReason::Manual);
    return true;
}

AutoSaveFrequency Game::getAutoSaveFrequency() const {
    return saveScheduler.getAutoSaveFrequency();
}

bool Game::setAutoSaveFrequency(AutoSaveFrequency frequency) {
    saveScheduler.setAutoSaveFrequency(frequency);
    persistSaveSettings();
    return true;
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
