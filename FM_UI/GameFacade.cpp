#include"GameFacade.h"
#include"Game.h"
#include"League.h"
#include"Team.h"
#include"Footballer.h"
#include"LeagueContext.h"
#include"GameInteraction.h"
#include"PostMatchInteraction.h"
#include"PreMatchInteraction.h"
#include"TransferOfferDecisionInteraction.h"
#include"TransferOffer.h"
#include"StandingsTableModel.h"
#include"TeamPlayersModel.h"
#include "TeamRecentMatchesModel.h"
#include "TeamUpcomingMatchesModel.h"
#include "PendingTransferOffersModel.h"
#include "DashboardStateObject.h"
#include "DashboardUpcomingMatchesModel.h"
#include "InteractionStateObject.h"
#include "ShellStateObject.h"

#include<QDebug>

#include<algorithm>
#include<exception>
#include<array>
#include<optional>
#include<string>
#include<vector>
#include<utility>

namespace {
    QVariantMap missingPlayerMap() {
        QVariantMap map;
        map.insert(QStringLiteral("hasPlayer"), false);
        return map;
    }

    QString fromStd(const std::string& value) {
        return QString::fromStdString(value);
    }

    QString monthName(Month month) {
        static const std::array<const char*, 12> names{
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };
        const int index = static_cast<int>(month) - 1;
        if (index < 0 || index >= static_cast<int>(names.size())) {
            return QStringLiteral("Unknown");
        }
        return QString::fromLatin1(names[static_cast<std::size_t>(index)]);
    }

    QString resultLetterForRecord(const MatchRecord& record, TeamId teamId) {
        const bool isHome = record.homeId == teamId;
        const int goalsFor = isHome ? record.homeGoals : record.awayGoals;
        const int goalsAgainst = isHome ? record.awayGoals : record.homeGoals;
        if (goalsFor > goalsAgainst) {
            return QStringLiteral("W");
        }
        if (goalsFor < goalsAgainst) {
            return QStringLiteral("L");
        }
        return QStringLiteral("D");
    }
}

GameFacade::GameFacade(QObject* parent)
    : QObject(parent),
      standingsModel(this),
      teamPlayersModel(this),
      teamRecentMatchesModel(this),
      teamUpcomingMatchesModel(this),
      pendingTransferOffersModel(this),
      currentTeamSeasonStatsObject(this),
      dashboardStateObject(this),
      dashboardUpcomingMatchesModel(this),
      shellStateObject(this),
      interactionStateObject(this) {
}

GameFacade::~GameFacade() {}

Game* GameFacade::ensureGame() {
    if (!game) {
        game = std::make_unique<Game>();
    }
    return game.get();
}

const Game* GameFacade::ensureGame() const {
    return const_cast<GameFacade*>(this)->ensureGame();
}

bool GameFacade::hasValidSelectedTeam() const {
    if (!gameStarted || selectedTeamId == 0 || selectedLeagueId == 0) {
        return false;
    }
    const League* league = resolveLeague(selectedLeagueId);
    return league && league->hasTeam(selectedTeamId);
}

bool GameFacade::hasValidLeagueSelection() const {
    return selectedLeagueId != 0 && resolveLeague(selectedLeagueId) != nullptr;
}

LeagueContext* GameFacade::resolveLeagueContext(LeagueId leagueId) {
    if (leagueId == 0) {
        return nullptr;
    }
    Game* currentGame = ensureGame();
    return currentGame ? currentGame->findLeagueContextById(leagueId) : nullptr;
}

const LeagueContext* GameFacade::resolveLeagueContext(LeagueId leagueId) const {
    return const_cast<GameFacade*>(this)->resolveLeagueContext(leagueId);
}

const League* GameFacade::resolveLeague(LeagueId leagueId) const {
    const LeagueContext* context = resolveLeagueContext(leagueId);
    return context ? &context->getLeague() : nullptr;
}

bool GameFacade::startNewGameInternal(LeagueId leagueId, TeamId teamId, const QString& newManagerName) {
    const QString trimmedManagerName = newManagerName.trimmed();
    if (leagueId == 0) {
        setLastError(QStringLiteral("Please choose a valid league."));
        qWarning() << "[GameFacade::startNewGame] Rejected invalid league id:" << leagueId;
        publishGameStateChanged();
        return false;
    }
    if (teamId == 0) {
        setLastError(QStringLiteral("Please choose a valid team."));
        qWarning() << "[GameFacade::startNewGame] Rejected invalid team id:" << teamId;
        publishGameStateChanged();
        return false;
    }
    if (trimmedManagerName.isEmpty()) {
        setLastError(QStringLiteral("Please enter a manager name."));
        qWarning() << "[GameFacade::startNewGame] Rejected empty manager name.";
        publishGameStateChanged();
        return false;
    }

    Game* currentGame = ensureGame();
    LeagueContext* selectedContext = currentGame ? currentGame->findLeagueContextById(leagueId) : nullptr;
    if (!selectedContext) {
        setLastError(QStringLiteral("Selected league could not be found."));
        qWarning() << "[GameFacade::startNewGame] League id is not present in active game:" << leagueId;
        publishGameStateChanged();
        return false;
    }

    Team* teamInNewGame = selectedContext->getLeague().findTeamById(teamId);
    if (!teamInNewGame) {
        setLastError(QStringLiteral("Selected team could not be found in the selected league."));
        qWarning() << "[GameFacade::startNewGame] Team id" << teamId << "not found in league" << leagueId;
        publishGameStateChanged();
        return false;
    }

    currentGame->setUserTeam(leagueId, teamInNewGame->getId());
    currentGame->pauseSimulation();
    selectedLeagueId = leagueId;
    selectedTeamId = teamInNewGame->getId();
    managerName = trimmedManagerName;
    gameStarted = true;
    setLastError(QString());
    qDebug() << "[GameFacade::startNewGame] Started new game with league" << selectedLeagueId << "team id" << selectedTeamId;
    publishGameStateChanged();
    return true;
}

void GameFacade::setLastError(const QString& errorMessage) {
    if (lastError == errorMessage) {
        return;
    }
    lastError = errorMessage;
    emit lastErrorChanged();
}

QVariantList GameFacade::getTeamSelectionList() const {

    struct TeamSummary {
        LeagueId leagueId = 0;
        TeamId teamId = 0;
        QString teamName;
        QString leagueName;
        int rating = 0;
        int squadSize = 0;
    };

    std::vector<TeamSummary> summaries;
    ensureGame()->forEachLeagueContext([&](const LeagueContext& context) {
        const League& league = context.getLeague();
        for (const auto& [teamId, team] : league.getTeams()) {
            (void)teamId;
            summaries.push_back(TeamSummary{
                league.getId(),
                team->getId(),
                fromStd(team->getName()),
                fromStd(league.getName()),
                team->calculateTeamRating(),
                static_cast<int>(team->playerCount())
                });
        }
        });
    

    std::sort(summaries.begin(), summaries.end(), [](const TeamSummary& lhs, const TeamSummary& rhs) {
        const int leagueCompare = lhs.leagueName.localeAwareCompare(rhs.leagueName);
        if (leagueCompare != 0) {
            return leagueCompare < 0;
        }   
        return lhs.teamName.localeAwareCompare(rhs.teamName) < 0;
        });

    QVariantList result;
    for (const TeamSummary& summary : summaries) {
        QVariantMap item;
        item.insert(QStringLiteral("leagueId"), static_cast<int>(summary.leagueId));
        item.insert(QStringLiteral("leagueName"), summary.leagueName);
        item.insert(QStringLiteral("teamId"), static_cast<int>(summary.teamId));
        item.insert(QStringLiteral("teamName"), summary.teamName);
        item.insert(QStringLiteral("shortRatingSummary"), QStringLiteral("OVR %1").arg(summary.rating));
        item.insert(QStringLiteral("squadSize"), summary.squadSize);
        result.push_back(item);
    }

    return result;
}

bool GameFacade::startNewGame(int teamId, const QString& newManagerName) {
    try {
        return startNewGameInternal(selectedLeagueId, static_cast<TeamId>(teamId), newManagerName);
    }
    catch (const std::exception& ex) {
        qWarning() << "[GameFacade::startNewGame] Exception:" << ex.what();
        if (!gameStarted) {
            selectedLeagueId = 0;
            selectedTeamId = 0;
            managerName.clear();
        }
        setLastError(QStringLiteral("Failed to start a new game. Please try again."));
        publishGameStateChanged();
        return false;
    }
    catch (...) {
        qWarning() << "[GameFacade::startNewGame] Unknown exception";
        if (!gameStarted) {
            selectedLeagueId = 0;
            selectedTeamId = 0;
            managerName.clear();
        }
        setLastError(QStringLiteral("Failed to start a new game. Please try again."));
        publishGameStateChanged();
        return false;
    }
}

bool GameFacade::startNewGameForLeagueTeam(int leagueId, int teamId, const QString& managerName) {
    try {
        return startNewGameInternal(static_cast<LeagueId>(leagueId), static_cast<TeamId>(teamId), managerName);
    }
    catch (const std::exception& ex) {
        qWarning() << "[GameFacade::startNewGameForLeagueTeam] Exception:" << ex.what();
        setLastError(QStringLiteral("Failed to start a new game. Please try again."));
        publishGameStateChanged();
        return false;
    }
    catch (...) {
        qWarning() << "[GameFacade::startNewGameForLeagueTeam] Unknown exception";
        setLastError(QStringLiteral("Failed to start a new game. Please try again."));
        publishGameStateChanged();
        return false;
    }
}

bool GameFacade::hasStartedGame() const {
    return gameStarted;
}

QString GameFacade::getCurrentDateText() const {
    if (!gameStarted) {
        return QString();
    }

    const Game* currentGame = ensureGame();
    return currentGame ? formatDate(currentGame->getDate()) : QString();
}

QString GameFacade::getCurrentStateText() const {
    if (!gameStarted) {
        return QString();
    }

    const Game* currentGame = ensureGame();
    return currentGame ? formatGameState(currentGame->getState()) : QString();
}

QString GameFacade::getSelectedTeamName() const {
    if (!hasValidSelectedTeam()) {
        return QString();
    }
    const League* league = resolveLeague(selectedLeagueId);
    return league ? fromStd(league->getTeamName(selectedTeamId)) : QString();
}

int GameFacade::getSelectedLeagueId() const {
    return static_cast<int>(selectedLeagueId);
}

int GameFacade::getSelectedTeamId() const {
    return static_cast<int>(selectedTeamId);
}

QString GameFacade::getManagerName() const {
    return managerName;
}

QString GameFacade::getLastError() const {
    return lastError;
}

void GameFacade::clearLastError() {
    setLastError(QString());
}

QAbstractListModel* GameFacade::getStandingsModel() const {
    return const_cast<StandingsTableModel*>(&standingsModel);
}

QAbstractListModel* GameFacade::getCurrentTeamPlayersModel() const {
    return const_cast<TeamPlayersModel*>(&teamPlayersModel);
}

QAbstractListModel* GameFacade::getCurrentTeamRecentMatchesModel() const {
    return const_cast<TeamRecentMatchesModel*>(&teamRecentMatchesModel);
}

QAbstractListModel* GameFacade::getCurrentTeamUpcomingMatchesModel() const {
    return const_cast<TeamUpcomingMatchesModel*>(&teamUpcomingMatchesModel);
}

QAbstractListModel* GameFacade::getPendingTransferOffersModel() const {
    return const_cast<PendingTransferOffersModel*>(&pendingTransferOffersModel);
}

TeamSeasonStatsObject* GameFacade::getCurrentTeamSeasonStatsObject() const {
    return const_cast<TeamSeasonStatsObject*>(&currentTeamSeasonStatsObject);
}

DashboardStateObject* GameFacade::getDashboardState() const {
    return const_cast<DashboardStateObject*>(&dashboardStateObject);
}

QAbstractListModel* GameFacade::getDashboardUpcomingMatchesModel() const {
    return const_cast<DashboardUpcomingMatchesModel*>(&dashboardUpcomingMatchesModel);
}

ShellStateObject* GameFacade::getShellState() const {
    return const_cast<ShellStateObject*>(&shellStateObject);
}

InteractionStateObject* GameFacade::getInteractionState() const {
    return const_cast<InteractionStateObject*>(&interactionStateObject);
}

QVariantMap GameFacade::getDashboard() const {
    QVariantMap dashboard;
    dashboard.insert(QStringLiteral("currentDateText"), getCurrentDateText());
    dashboard.insert(QStringLiteral("gameStateText"), getCurrentStateText());
    dashboard.insert(QStringLiteral("selectedTeamName"), getSelectedTeamName());
    dashboard.insert(QStringLiteral("selectedTeamId"), getSelectedTeamId());
    dashboard.insert(QStringLiteral("recentForm"), QString());

    QVariantMap emptyNextMatch;
    emptyNextMatch.insert(QStringLiteral("hasNextMatch"), false);
    dashboard.insert(QStringLiteral("nextMatch"), emptyNextMatch);
    dashboard.insert(QStringLiteral("standingsRow"), QVariantMap{});
    dashboard.insert(QStringLiteral("shortTeamStats"), QVariantMap{});
    dashboard.insert(QStringLiteral("upcomingMatches"), QVariantList{});

    if (!hasValidSelectedTeam()) {
        return dashboard;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return dashboard;
    }

    dashboard.insert(QStringLiteral("recentForm"),
        fromStd(league->getRecentFormString(selectedTeamId, 5)));

    dashboard.insert(QStringLiteral("upcomingMatches"),
        getCurrentTeamUpcomingMatches(3));

    if (const std::optional<FixtureMatchPreview> nextMatch =
        league->getNextMatchForTeam(selectedTeamId);
        nextMatch.has_value()) {
        dashboard.insert(QStringLiteral("nextMatch"), toNextMatchMap(*nextMatch));
    }

    const std::vector<StandingsEntry> standings = league->getSortedStandings();
    for (std::size_t index = 0; index < standings.size(); ++index) {
        if (standings[index].teamId == selectedTeamId) {
            const StandingsEntry& entry = standings[index];
            QVariantMap standingsRow;
            standingsRow.insert(QStringLiteral("position"), static_cast<int>(index + 1));
            standingsRow.insert(QStringLiteral("teamId"), static_cast<int>(entry.teamId));
            standingsRow.insert(QStringLiteral("teamName"), fromStd(league->getTeamName(entry.teamId)));
            standingsRow.insert(QStringLiteral("played"), entry.played);
            standingsRow.insert(QStringLiteral("wins"), entry.wins);
            standingsRow.insert(QStringLiteral("draws"), entry.draws);
            standingsRow.insert(QStringLiteral("losses"), entry.losses);
            standingsRow.insert(QStringLiteral("goalsFor"), entry.goalsFor);
            standingsRow.insert(QStringLiteral("goalsAgainst"), entry.goalsAgainst);
            standingsRow.insert(QStringLiteral("goalDifference"), entry.goalDifference);
            standingsRow.insert(QStringLiteral("points"), entry.points);
            dashboard.insert(QStringLiteral("standingsRow"), standingsRow);
            break;
        }
    }

    if (const TeamSeasonStats* stats =
        league->getCurrentTeamSeasonStatsFor(selectedTeamId)) {
        dashboard.insert(QStringLiteral("shortTeamStats"), toTeamStatsMap(*stats));
    }

    return dashboard;
}

bool GameFacade::advanceOneDay() {
    return advanceDays(1);
}

bool GameFacade::advanceDays(int count) {
    if (!gameStarted || count <= 0) {
        return false;
    }

    if (count > 3650) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    for (int i = 0; i < count; ++i) {
        currentGame->updateDaily();
    }

    publishGameStateChanged();
    return true;
}

bool GameFacade::isTimePaused() const {
    if (!gameStarted) {
        return true;
    }

    const Game* currentGame = ensureGame();
    return currentGame ? currentGame->isTimePaused() : true;
}

bool GameFacade::pauseSimulation() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const bool paused = currentGame->pauseSimulation();
    publishGameStateChanged();
    return paused;
}

bool GameFacade::resumeSimulation() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const bool resumed = currentGame->resumeSimulation();
    publishGameStateChanged();
    return resumed;
}

bool GameFacade::hasActiveInteraction() const {
    if (!gameStarted) {
        return false;
    }
    const Game* currentGame = ensureGame();
    return currentGame ? currentGame->hasActiveBlockingInteraction() : false;
}

QString GameFacade::getActiveInteractionKind() const {
    if (!gameStarted) {
        return QString();
    }

    const Game* currentGame = ensureGame();
    if (!currentGame || !currentGame->hasActiveBlockingInteraction()) {
        return QString();
    }

    const GameInteraction* interaction = currentGame->getActiveInteraction();
    if (!interaction) {
        return QString();
    }

    switch (interaction->getKind()) {
    case GameInteraction::Kind::PreMatch:
        return QStringLiteral("pre_match");
    case GameInteraction::Kind::PostMatch:
        return QStringLiteral("post_match");
    case GameInteraction::Kind::TransferOfferDecision:
        return QStringLiteral("transfer_offer");
    }

    return QString();
}

QVariantMap GameFacade::getActivePreMatchInteraction() const {
    if (!gameStarted) {
        return {};
    }

    const Game* currentGame = ensureGame();
    if (!currentGame) {
        return {};
    }

    const PreMatchInteraction* interaction = currentGame->getActivePreMatchInteraction();
    if (!interaction) {
        return {};
    }

    return toPreMatchInteractionMap(*interaction);
}

QVariantMap GameFacade::getActivePostMatchInteraction() const {
    if (!gameStarted) {
        return {};
    }

    const Game* currentGame = ensureGame();
    if (!currentGame) {
        return {};
    }

    const PostMatchInteraction* interaction = currentGame->getActivePostMatchInteraction();
    if (!interaction) {
        return {};
    }

    return toPostMatchInteractionMap(*interaction);
}

QVariantMap GameFacade::getActiveTransferOfferInteraction() const {
    if (!gameStarted) {
        return {};
    }

    const Game* currentGame = ensureGame();
    if (!currentGame) {
        return {};
    }

    const TransferOfferDecisionInteraction* interaction = currentGame->getActiveTransferOfferDecisionInteraction();
    if (!interaction) {
        return {};
    }

    return toTransferOfferInteractionMap(*interaction);
}

bool GameFacade::resolveActiveInteraction() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const bool resolved = currentGame->resolveActiveInteraction();
    publishGameStateChanged();
    return resolved;
}

bool GameFacade::playActiveMatch() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const bool played = currentGame->playPendingPreMatch();
    publishGameStateChanged();
    return played;
}

bool GameFacade::acceptActiveTransferOffer() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const TransferOfferDecisionInteraction* interaction = currentGame->getActiveTransferOfferDecisionInteraction();
    if (!interaction) {
        publishGameStateChanged();
        return false;
    }

    const bool accepted = currentGame->acceptTransferOffer(interaction->getOfferId());
    publishGameStateChanged();
    return accepted;
}

bool GameFacade::rejectActiveTransferOffer() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const TransferOfferDecisionInteraction* interaction = currentGame->getActiveTransferOfferDecisionInteraction();
    if (!interaction) {
        publishGameStateChanged();
        return false;
    }

    const bool rejected = currentGame->rejectTransferOffer(interaction->getOfferId());
    publishGameStateChanged();
    return rejected;
}

bool GameFacade::deferActiveTransferOffer() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    if (!currentGame->getActiveTransferOfferDecisionInteraction()) {
        publishGameStateChanged();
        return false;
    }

    const bool deferred = currentGame->resolveActiveInteraction();
    publishGameStateChanged();
    return deferred;
}

QVariantList GameFacade::getPendingTransferOffers() const {
    QVariantList pendingOffers;
    if (!gameStarted) {
        return pendingOffers;
    }

    const Game* currentGame = ensureGame();
    if (!currentGame) {
        return pendingOffers;
    }

    const std::vector<const TransferOffer*> offers = currentGame->getPendingTransferOffersForManagedTeam();
    for (const TransferOffer* offer : offers) {
        if (!offer) {
            continue;
        }
        pendingOffers.push_back(toPendingTransferOfferMap(*offer));
    }

    return pendingOffers;
}

bool GameFacade::acceptTransferOfferById(int offerId) {
    if (!gameStarted || offerId <= 0) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const bool accepted = currentGame->acceptTransferOffer(static_cast<OfferId>(offerId));
    publishGameStateChanged();
    return accepted;
}

bool GameFacade::rejectTransferOfferById(int offerId) {
    if (!gameStarted || offerId <= 0) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    const bool rejected = currentGame->rejectTransferOffer(static_cast<OfferId>(offerId));
    publishGameStateChanged();
    return rejected;
}


QVariantList GameFacade::getStandingsTable() const {
    QVariantList table;
    if (!gameStarted) {
        return table;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return table;
    }

    const std::vector<StandingsEntry> standings = league->getSortedStandings();
    for (std::size_t index = 0; index < standings.size(); ++index) {
        const StandingsEntry& entry = standings[index];
        QVariantMap row;
        row.insert(QStringLiteral("position"), static_cast<int>(index + 1));
        row.insert(QStringLiteral("teamId"), static_cast<int>(entry.teamId));
        row.insert(QStringLiteral("teamName"), fromStd(league->getTeamName(entry.teamId)));
        row.insert(QStringLiteral("played"), entry.played);
        row.insert(QStringLiteral("wins"), entry.wins);
        row.insert(QStringLiteral("draws"), entry.draws);
        row.insert(QStringLiteral("losses"), entry.losses);
        row.insert(QStringLiteral("goalsFor"), entry.goalsFor);
        row.insert(QStringLiteral("goalsAgainst"), entry.goalsAgainst);
        row.insert(QStringLiteral("goalDifference"), entry.goalDifference);
        row.insert(QStringLiteral("points"), entry.points);
        row.insert(QStringLiteral("recentForm"), fromStd(league->getRecentFormString(entry.teamId, 5)));
        row.insert(QStringLiteral("isSelectedTeam"), entry.teamId == selectedTeamId);
        table.push_back(row);
    }

    return table;
}

QVariantMap GameFacade::getCurrentTeamSeasonStats() const {
    if (!hasValidSelectedTeam()) {
        return {};
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return {};
    }
    const TeamSeasonStats* stats = league->getCurrentTeamSeasonStatsFor(selectedTeamId);
    return stats ? toTeamStatsMap(*stats) : QVariantMap{};
}

QVariantList GameFacade::getCurrentTeamMatches() const {
    QVariantList matches;
    if (!hasValidSelectedTeam()) {
        return matches;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return matches;
    }
    std::vector<MatchRecord> records = league->getMatchesForTeamInCurrentSeason(selectedTeamId);
    std::reverse(records.begin(), records.end());
    for (const MatchRecord& record : records) {
        matches.push_back(toMatchRecordMap(record));
    }
    return matches;
}

QVariantList GameFacade::getCurrentTeamUpcomingMatches(int count) const {
    QVariantList matches;
    if (!hasValidSelectedTeam() || count <= 0) {
        return matches;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return matches;
    }
    const auto previews = league->getUpcomingMatchesForTeam(
        selectedTeamId,
        static_cast<std::size_t>(count)
    );

    for (const auto& preview : previews) {
        matches.push_back(toNextMatchMap(preview));
    }

    return matches;
}

QVariantList GameFacade::getCurrentTeamPlayers() const {
    QVariantList players;
    if (!hasValidSelectedTeam()) {
        return players;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return players;
    }

    const Team* team = league->findTeamById(selectedTeamId);
    if (!team) {
        return players;
    }

    for (const auto& player : team->getPlayers()) {
        players.push_back(toPlayerMap(*player));
    }
    return players;
}

QVariantMap GameFacade::getPlayerDetails(int playerId) const {
    if (!gameStarted || playerId <= 0) {
        return missingPlayerMap();
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return missingPlayerMap();
    }
    const Footballer* player = league->findPlayerById(static_cast<PlayerId>(playerId));
    if (!player) {
        return missingPlayerMap();
    }

    return toPlayerDetailsMap(*player);
}

void GameFacade::refreshStandingsModel() {
    if (!gameStarted) {
        standingsModel.clear();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        standingsModel.clear();
        return;
    }

    const std::vector<StandingsEntry> standings = league->getSortedStandings();
    QVector<StandingsTableModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(standings.size()));

    for (std::size_t index = 0; index < standings.size(); ++index) {
        const StandingsEntry& entry = standings[index];
        StandingsTableModel::Row row;
        row.position = static_cast<int>(index + 1);
        row.teamId = static_cast<int>(entry.teamId);
        row.teamName = fromStd(league->getTeamName(entry.teamId));
        row.played = entry.played;
        row.wins = entry.wins;
        row.draws = entry.draws;
        row.losses = entry.losses;
        row.goalsFor = entry.goalsFor;
        row.goalsAgainst = entry.goalsAgainst;
        row.goalDifference = entry.goalDifference;
        row.points = entry.points;
        row.recentForm = fromStd(league->getRecentFormString(entry.teamId, 5));
        row.isSelectedTeam = entry.teamId == selectedTeamId;
        rows.push_back(std::move(row));
    }

    standingsModel.setRows(std::move(rows));
}

void GameFacade::refreshCurrentTeamPlayersModel() {
    if (!hasValidSelectedTeam()) {
        teamPlayersModel.clear();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        teamPlayersModel.clear();
        return;
    }

    const Team* team = league->findTeamById(selectedTeamId);
    if (!team) {
        teamPlayersModel.clear();
        return;
    }

    QVector<TeamPlayersModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(team->getPlayers().size()));
    for (const auto& player : team->getPlayers()) {
        const PlayerSeasonStats& stats = player->getCurrentSeasonStats();
        TeamPlayersModel::Row row;
        row.playerId = static_cast<int>(player->getId());
        row.name = fromStd(player->getName());
        row.age = player->getAge();
        row.position = fromStd(player->getPosition());
        row.overallSummary = QStringLiteral("OVR %1").arg(player->totalPower());
        row.appearances = stats.appearances;
        row.minutes = stats.minutesPlayed;
        row.goals = stats.goals;
        row.assists = stats.assists;
        rows.push_back(std::move(row));
    }

    teamPlayersModel.setRows(std::move(rows));
}

void GameFacade::refreshCurrentTeamRecentMatchesModel() {
    if (!hasValidSelectedTeam()) {
        teamRecentMatchesModel.clear();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        teamRecentMatchesModel.clear();
        return;
    }

    std::vector<MatchRecord> records = league->getMatchesForTeamInCurrentSeason(selectedTeamId);
    std::reverse(records.begin(), records.end());

    QVector<TeamRecentMatchesModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(records.size()));

    for (const MatchRecord& record : records) {
        const bool isHome = record.homeId == selectedTeamId;
        const TeamId opponentId = isHome ? record.awayId : record.homeId;
        const int goalsFor = isHome ? record.homeGoals : record.awayGoals;
        const int goalsAgainst = isHome ? record.awayGoals : record.homeGoals;

        TeamRecentMatchesModel::Row row;
        row.dateText = formatDate(record.date);
        row.matchweek = record.matchweek;
        row.opponentId = static_cast<int>(opponentId);
        row.opponentName = fromStd(league->getTeamName(opponentId));
        row.isHome = isHome;
        row.goalsFor = goalsFor;
        row.goalsAgainst = goalsAgainst;
        row.resultLetter = resultLetterForRecord(record, selectedTeamId);
        rows.push_back(std::move(row));
    }

    teamRecentMatchesModel.setRows(std::move(rows));
}

void GameFacade::refreshCurrentTeamUpcomingMatchesModel() {
    if (!hasValidSelectedTeam()) {
        teamUpcomingMatchesModel.clear();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        teamUpcomingMatchesModel.clear();
        return;
    }

    const auto previews = league->getUpcomingMatchesForTeam(selectedTeamId, 5);
    QVector<TeamUpcomingMatchesModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(previews.size()));

    for (const auto& preview : previews) {
        TeamUpcomingMatchesModel::Row row;
        row.dateText = formatDate(preview.date);
        row.homeTeamId = static_cast<int>(preview.homeId);
        row.awayTeamId = static_cast<int>(preview.awayId);
        row.homeTeamName = fromStd(league->getTeamName(preview.homeId));
        row.awayTeamName = fromStd(league->getTeamName(preview.awayId));
        row.isHome = preview.homeId == selectedTeamId;
        row.matchweek = preview.matchweek;
        rows.push_back(std::move(row));
    }

    teamUpcomingMatchesModel.setRows(std::move(rows));
}

void GameFacade::refreshCurrentTeamSeasonStatsObject() {
    if (!hasValidSelectedTeam()) {
        currentTeamSeasonStatsObject.clear();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        currentTeamSeasonStatsObject.clear();
        return;
    }

    const TeamSeasonStats* stats = league->getCurrentTeamSeasonStatsFor(selectedTeamId);
    if (!stats) {
        currentTeamSeasonStatsObject.clear();
        return;
    }

    currentTeamSeasonStatsObject.setFrom(*stats);
}

void GameFacade::refreshPendingTransferOffersModel() {
    if (!gameStarted) {
        pendingTransferOffersModel.clear();
        return;
    }

    const Game* currentGame = ensureGame();
    if (!currentGame) {
        pendingTransferOffersModel.clear();
        return;
    }

    const std::vector<const TransferOffer*> offers = currentGame->getPendingTransferOffersForManagedTeam();
    QVector<PendingTransferOffersModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(offers.size()));

    for (const TransferOffer* offer : offers) {
        if (!offer) {
            continue;
        }

        PendingTransferOffersModel::Row row;
        row.offerId = static_cast<int>(offer->id);
        row.playerId = static_cast<int>(offer->playerId);
        row.buyerTeamId = static_cast<int>(offer->buyerTeamId);
        row.fee = static_cast<qlonglong>(offer->fee);
        row.lastDateText = formatDate(offer->lastValidDate);

        const League* sellerLeague = resolveLeague(offer->sellerLeagueId);
        const League* buyerLeague = resolveLeague(offer->buyerLeagueId);
        row.buyerTeamName = buyerLeague ? fromStd(buyerLeague->getTeamName(offer->buyerTeamId)) : QString();

        if (sellerLeague) {
            if (const Footballer* player = sellerLeague->findPlayerById(offer->playerId)) {
                row.playerName = fromStd(player->getName());
            }
        }

        rows.push_back(std::move(row));
    }

    pendingTransferOffersModel.setRows(std::move(rows));
}

void GameFacade::refreshDashboardState() {
    if (!hasValidSelectedTeam()) {
        dashboardStateObject.clear();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        dashboardStateObject.clear();
        return;
    }

    const QString selectedTeamName = fromStd(league->getTeamName(selectedTeamId));
    dashboardStateObject.setRootValues(
        true,
        selectedTeamName,
        formatDate(ensureGame()->getDate()),
        formatGameState(ensureGame()->getState()),
        managerName,
        fromStd(league->getRecentFormString(selectedTeamId, 5))
    );

    if (const std::optional<FixtureMatchPreview> nextMatch = league->getNextMatchForTeam(selectedTeamId);
        nextMatch.has_value()) {
        dashboardStateObject.nextMatch()->setFromValues(
            formatDate(nextMatch->date),
            fromStd(league->getTeamName(nextMatch->homeId)),
            fromStd(league->getTeamName(nextMatch->awayId)),
            nextMatch->homeId == selectedTeamId,
            nextMatch->matchweek
        );
    } else {
        dashboardStateObject.nextMatch()->clear();
    }

    if (const TeamSeasonStats* stats = league->getCurrentTeamSeasonStatsFor(selectedTeamId)) {
        dashboardStateObject.teamStats()->setFrom(*stats);
    } else {
        dashboardStateObject.teamStats()->clear();
    }

    const std::vector<StandingsEntry> standings = league->getSortedStandings();
    bool hasStandingsRow = false;
    for (std::size_t index = 0; index < standings.size(); ++index) {
        const StandingsEntry& entry = standings[index];
        if (entry.teamId != selectedTeamId) {
            continue;
        }
        dashboardStateObject.standingsRow()->setFromValues(
            static_cast<int>(index + 1),
            selectedTeamName,
            entry.played,
            entry.wins,
            entry.draws,
            entry.losses,
            entry.goalDifference,
            entry.points
        );
        hasStandingsRow = true;
        break;
    }

    if (!hasStandingsRow) {
        dashboardStateObject.standingsRow()->clear();
    }
}

void GameFacade::refreshDashboardUpcomingMatchesModel() {
    if (!hasValidSelectedTeam()) {
        dashboardUpcomingMatchesModel.clear();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        dashboardUpcomingMatchesModel.clear();
        return;
    }

    const auto previews = league->getUpcomingMatchesForTeam(selectedTeamId, 3);
    QVector<DashboardUpcomingMatchesModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(previews.size()));

    for (const auto& preview : previews) {
        DashboardUpcomingMatchesModel::Row row;
        row.dateText = formatDate(preview.date);
        row.homeTeamName = fromStd(league->getTeamName(preview.homeId));
        row.awayTeamName = fromStd(league->getTeamName(preview.awayId));
        row.isHome = preview.homeId == selectedTeamId;
        row.matchweek = preview.matchweek;
        rows.push_back(std::move(row));
    }

    dashboardUpcomingMatchesModel.setRows(std::move(rows));
}

void GameFacade::refreshShellStateObject() {
    if (!gameStarted) {
        shellStateObject.clear();
        return;
    }

    const Game* currentGame = ensureGame();
    if (!currentGame) {
        shellStateObject.clear();
        return;
    }

    shellStateObject.setFromValues(
        true,
        getSelectedTeamName(),
        formatDate(currentGame->getDate()),
        formatGameState(currentGame->getState()),
        managerName,
        currentGame->isTimePaused()
    );
}

void GameFacade::refreshInteractionStateObject() {
    if (!gameStarted) {
        interactionStateObject.clear();
        return;
    }

    const Game* currentGame = ensureGame();
    if (!currentGame || !currentGame->hasActiveBlockingInteraction()) {
        interactionStateObject.clear();
        return;
    }

    const GameInteraction* interaction = currentGame->getActiveInteraction();
    if (!interaction) {
        interactionStateObject.clear();
        return;
    }

    switch (interaction->getKind()) {
    case GameInteraction::Kind::PreMatch: {
        const PreMatchInteraction* preMatch = currentGame->getActivePreMatchInteraction();
        if (!preMatch) {
            interactionStateObject.clear();
            return;
        }

        const League* league = resolveLeague(preMatch->getLeagueId());
        interactionStateObject.preMatch()->setFromValues(
            formatDate(preMatch->getDate()),
            preMatch->getMatchweek(),
            static_cast<int>(preMatch->getHomeId()),
            static_cast<int>(preMatch->getAwayId()),
            league ? fromStd(league->getTeamName(preMatch->getHomeId())) : QString(),
            league ? fromStd(league->getTeamName(preMatch->getAwayId())) : QString()
        );
        interactionStateObject.postMatch()->clear();
        interactionStateObject.transferOffer()->clear();
        interactionStateObject.setFromValues(true, QStringLiteral("pre_match"));
        return;
    }
    case GameInteraction::Kind::PostMatch: {
        const PostMatchInteraction* postMatch = currentGame->getActivePostMatchInteraction();
        if (!postMatch) {
            interactionStateObject.clear();
            return;
        }

        const League* league = resolveLeague(postMatch->getLeagueId());
        interactionStateObject.postMatch()->setFromValues(
            formatDate(postMatch->getDate()),
            postMatch->getMatchweek(),
            static_cast<int>(postMatch->getHomeId()),
            static_cast<int>(postMatch->getAwayId()),
            league ? fromStd(league->getTeamName(postMatch->getHomeId())) : QString(),
            league ? fromStd(league->getTeamName(postMatch->getAwayId())) : QString(),
            postMatch->getHomeGoals(),
            postMatch->getAwayGoals()
        );
        interactionStateObject.preMatch()->clear();
        interactionStateObject.transferOffer()->clear();
        interactionStateObject.setFromValues(true, QStringLiteral("post_match"));
        return;
    }
    case GameInteraction::Kind::TransferOfferDecision: {
        const TransferOfferDecisionInteraction* transferOffer = currentGame->getActiveTransferOfferDecisionInteraction();
        if (!transferOffer) {
            interactionStateObject.clear();
            return;
        }

        const League* sellerLeague = resolveLeague(transferOffer->getSellerLeagueId());
        const League* buyerLeague = resolveLeague(transferOffer->getBuyerLeagueId());
        QString playerName;
        if (sellerLeague) {
            if (const Footballer* player = sellerLeague->findPlayerById(transferOffer->getPlayerId())) {
                playerName = fromStd(player->getName());
            }
        }

        interactionStateObject.transferOffer()->setFromValues(
            static_cast<int>(transferOffer->getOfferId()),
            static_cast<int>(transferOffer->getPlayerId()),
            playerName,
            sellerLeague ? fromStd(sellerLeague->getTeamName(transferOffer->getSellerTeamId())) : QString(),
            buyerLeague ? fromStd(buyerLeague->getTeamName(transferOffer->getBuyerTeamId())) : QString(),
            static_cast<qlonglong>(transferOffer->getFee())
        );
        interactionStateObject.preMatch()->clear();
        interactionStateObject.postMatch()->clear();
        interactionStateObject.setFromValues(true, QStringLiteral("transfer_offer"));
        return;
    }
    }
}

void GameFacade::publishGameStateChanged() {
    refreshStandingsModel();
    refreshCurrentTeamPlayersModel();
    refreshCurrentTeamRecentMatchesModel();
    refreshCurrentTeamUpcomingMatchesModel();
    refreshPendingTransferOffersModel();
    refreshCurrentTeamSeasonStatsObject();
    refreshDashboardState();
    refreshDashboardUpcomingMatchesModel();
    refreshShellStateObject();
    refreshInteractionStateObject();
    emit gameStateChanged();
}

QString GameFacade::formatDate(const Date& date) const {
    return QStringLiteral("%1 %2, %3")
        .arg(monthName(date.getMonth()))
        .arg(date.getDay())
        .arg(date.getYear());
}

QString GameFacade::formatGameState(GameState state) const {
    switch (state) {
    case GameState::PreSeason:
        return QStringLiteral("Pre-Season");
    case GameState::InSeason:
        return QStringLiteral("In Season");
    case GameState::PostSeason:
        return QStringLiteral("Post-Season");
    default:
        return QStringLiteral("Unknown");
    }
}

QString GameFacade::formatTransferOfferExpiryPolicy(TransferOfferExpiryPolicy policy) const {
    switch (policy) {
    case TransferOfferExpiryPolicy::FourteenDayLimit:
        return QStringLiteral("14-day limit");
    case TransferOfferExpiryPolicy::WindowCloseLimit:
        return QStringLiteral("Window close limit");
    default:
        break;
    }
    return QStringLiteral("Unknown");
}

QVariantMap GameFacade::toNextMatchMap(const FixtureMatchPreview& preview) const {
    QVariantMap map;
    const League* league = resolveLeague(selectedLeagueId); 
    map.insert(QStringLiteral("hasNextMatch"), true);
    map.insert(QStringLiteral("dateText"), formatDate(preview.date));
    map.insert(QStringLiteral("homeTeamId"), static_cast<int>(preview.homeId));
    map.insert(QStringLiteral("awayTeamId"), static_cast<int>(preview.awayId));
    map.insert(QStringLiteral("homeTeamName"), league ? fromStd(league->getTeamName(preview.homeId)) : QString());
    map.insert(QStringLiteral("awayTeamName"), league ? fromStd(league->getTeamName(preview.awayId)) : QString());
    map.insert(QStringLiteral("isHome"), preview.homeId == selectedTeamId);
    map.insert(QStringLiteral("matchweek"), preview.matchweek);
    return map;
}

QVariantMap GameFacade::toTeamStatsMap(const TeamSeasonStats& stats) const {
    QVariantMap map;
    map.insert(QStringLiteral("teamId"), static_cast<int>(stats.teamId));
    map.insert(QStringLiteral("seasonYear"), stats.seasonYear);
    map.insert(QStringLiteral("played"), stats.played);
    map.insert(QStringLiteral("wins"), stats.wins);
    map.insert(QStringLiteral("draws"), stats.draws);
    map.insert(QStringLiteral("losses"), stats.losses);
    map.insert(QStringLiteral("goalsFor"), stats.goalsFor);
    map.insert(QStringLiteral("goalsAgainst"), stats.goalsAgainst);
    map.insert(QStringLiteral("goalDifference"), stats.goalsFor - stats.goalsAgainst);
    map.insert(QStringLiteral("points"), stats.points);
    map.insert(QStringLiteral("cleanSheets"), stats.cleanSheets);
    map.insert(QStringLiteral("failedToScore"), stats.failedToScore);
    map.insert(QStringLiteral("homePlayed"), stats.homePlayed);
    map.insert(QStringLiteral("awayPlayed"), stats.awayPlayed);
    return map;
}

QVariantMap GameFacade::toMatchRecordMap(const MatchRecord& record) const {
    QVariantMap map;
    const League* league = resolveLeague(selectedLeagueId);
    const bool isHome = record.homeId == selectedTeamId;
    const TeamId opponentId = isHome ? record.awayId : record.homeId;
    const int goalsFor = isHome ? record.homeGoals : record.awayGoals;
    const int goalsAgainst = isHome ? record.awayGoals : record.homeGoals;

    map.insert(QStringLiteral("dateText"), formatDate(record.date));
    map.insert(QStringLiteral("matchweek"), record.matchweek);
    map.insert(QStringLiteral("opponentId"), static_cast<int>(opponentId));
    map.insert(QStringLiteral("opponentName"), league ? fromStd(league->getTeamName(opponentId)) : QString());
    map.insert(QStringLiteral("isHome"), isHome);
    map.insert(QStringLiteral("goalsFor"), goalsFor);
    map.insert(QStringLiteral("goalsAgainst"), goalsAgainst);
    map.insert(QStringLiteral("resultLetter"), resultLetterForRecord(record, selectedTeamId));
    return map;
}

QVariantMap GameFacade::toPlayerSeasonStatsMap(const PlayerSeasonStats& stats) const {
    QVariantMap map;
    map.insert(QStringLiteral("playerId"), static_cast<int>(stats.playerId));
    map.insert(QStringLiteral("seasonYear"), stats.seasonYear);
    map.insert(QStringLiteral("appearances"), stats.appearances);
    map.insert(QStringLiteral("starts"), stats.starts);
    map.insert(QStringLiteral("substituteAppearances"), stats.substituteAppearances);
    map.insert(QStringLiteral("minutes"), stats.minutesPlayed);
    map.insert(QStringLiteral("goals"), stats.goals);
    map.insert(QStringLiteral("assists"), stats.assists);
    map.insert(QStringLiteral("yellowCards"), stats.yellowCards);
    map.insert(QStringLiteral("redCards"), stats.redCards);
    map.insert(QStringLiteral("cleanSheets"), stats.cleanSheets);
    map.insert(QStringLiteral("goalsConcededWhileOnPitch"), stats.goalsConcededWhileOnPitch);
    return map;
}

QVariantMap GameFacade::toPlayerMap(const Footballer& player) const {
    QVariantMap map;
    const PlayerSeasonStats& stats = player.getCurrentSeasonStats();
    map.insert(QStringLiteral("playerId"), static_cast<int>(player.getId()));
    map.insert(QStringLiteral("name"), fromStd(player.getName()));
    map.insert(QStringLiteral("age"), player.getAge());
    map.insert(QStringLiteral("position"), fromStd(player.getPosition()));
    map.insert(QStringLiteral("overallSummary"), QStringLiteral("OVR %1").arg(player.totalPower()));
    map.insert(QStringLiteral("appearances"), stats.appearances);
    map.insert(QStringLiteral("minutes"), stats.minutesPlayed);
    map.insert(QStringLiteral("goals"), stats.goals);
    map.insert(QStringLiteral("assists"), stats.assists);
    return map;
}

QVariantMap GameFacade::toPlayerDetailsMap(const Footballer& player) const {
    QVariantMap map = toPlayerMap(player);
    map.insert(QStringLiteral("hasPlayer"), true);
    map.insert(QStringLiteral("teamId"), static_cast<int>(player.getTeamId()));

    const League* league = resolveLeague(selectedLeagueId);
    if (league && player.getTeamId() != 0 && league->hasTeam(player.getTeamId())) {
        map.insert(QStringLiteral("teamName"), fromStd(league->getTeamName(player.getTeamId())));
    }
    else {
        map.insert(QStringLiteral("teamName"), QStringLiteral("Free Agent"));
    }

    map.insert(QStringLiteral("currentSeasonStats"),
        toPlayerSeasonStatsMap(player.getCurrentSeasonStats()));

    QVariantList archivedSeasons;
    int archivedAppearances = 0;
    int archivedGoals = 0;
    int archivedAssists = 0;

    std::vector<int> archivedYears;
    archivedYears.reserve(player.getArchivedSeasonStatsByYear().size());
    for (const auto& [seasonYear, stats] : player.getArchivedSeasonStatsByYear()) {
        (void)stats;
        archivedYears.push_back(seasonYear);
    }
    std::sort(archivedYears.begin(), archivedYears.end(), std::greater<int>{});

    for (int seasonYear : archivedYears) {
        const PlayerSeasonStats& stats = player.getArchivedSeasonStatsByYear().at(seasonYear);
        QVariantMap archived = toPlayerSeasonStatsMap(stats);
        archived.insert(QStringLiteral("seasonYear"), seasonYear);
        archivedSeasons.push_back(archived);
        archivedAppearances += stats.appearances;
        archivedGoals += stats.goals;
        archivedAssists += stats.assists;
    }

    QVariantMap archivedSummary;
    archivedSummary.insert(QStringLiteral("seasonCount"),
        static_cast<int>(player.getArchivedSeasonStatsByYear().size()));
    archivedSummary.insert(QStringLiteral("totalAppearances"), archivedAppearances);
    archivedSummary.insert(QStringLiteral("totalGoals"), archivedGoals);
    archivedSummary.insert(QStringLiteral("totalAssists"), archivedAssists);
    archivedSummary.insert(QStringLiteral("seasons"), archivedSeasons);
    map.insert(QStringLiteral("archivedStatsSummary"), archivedSummary);

    return map;
}

QVariantMap GameFacade::toPreMatchInteractionMap(const PreMatchInteraction& interaction) const {
    QVariantMap map;
    map.insert(QStringLiteral("kind"), QStringLiteral("pre_match"));
    map.insert(QStringLiteral("dateText"), formatDate(interaction.getDate()));
    map.insert(QStringLiteral("matchweek"), interaction.getMatchweek());
    map.insert(QStringLiteral("homeTeamId"), static_cast<int>(interaction.getHomeId()));
    map.insert(QStringLiteral("awayTeamId"), static_cast<int>(interaction.getAwayId()));

    const League* league = resolveLeague(interaction.getLeagueId());
    map.insert(QStringLiteral("homeTeamName"), league ? fromStd(league->getTeamName(interaction.getHomeId())) : QString());
    map.insert(QStringLiteral("awayTeamName"), league ? fromStd(league->getTeamName(interaction.getAwayId())) : QString());
    return map;
}

QVariantMap GameFacade::toPostMatchInteractionMap(const PostMatchInteraction& interaction) const {
    QVariantMap map;
    map.insert(QStringLiteral("kind"), QStringLiteral("post_match"));
    map.insert(QStringLiteral("dateText"), formatDate(interaction.getDate()));
    map.insert(QStringLiteral("matchweek"), interaction.getMatchweek());
    map.insert(QStringLiteral("homeTeamId"), static_cast<int>(interaction.getHomeId()));
    map.insert(QStringLiteral("awayTeamId"), static_cast<int>(interaction.getAwayId()));
    map.insert(QStringLiteral("homeGoals"), interaction.getHomeGoals());
    map.insert(QStringLiteral("awayGoals"), interaction.getAwayGoals());

    const League* league = resolveLeague(interaction.getLeagueId());
    map.insert(QStringLiteral("homeTeamName"), league ? fromStd(league->getTeamName(interaction.getHomeId())) : QString());
    map.insert(QStringLiteral("awayTeamName"), league ? fromStd(league->getTeamName(interaction.getAwayId())) : QString());
    return map;
}

QVariantMap GameFacade::toTransferOfferInteractionMap(const TransferOfferDecisionInteraction& interaction) const {
    QVariantMap map;
    map.insert(QStringLiteral("kind"), QStringLiteral("transfer_offer"));
    map.insert(QStringLiteral("offerId"), static_cast<int>(interaction.getOfferId()));
    map.insert(QStringLiteral("sellerLeagueId"), static_cast<int>(interaction.getSellerLeagueId()));
    map.insert(QStringLiteral("sellerTeamId"), static_cast<int>(interaction.getSellerTeamId()));
    map.insert(QStringLiteral("buyerLeagueId"), static_cast<int>(interaction.getBuyerLeagueId()));
    map.insert(QStringLiteral("buyerTeamId"), static_cast<int>(interaction.getBuyerTeamId()));
    map.insert(QStringLiteral("playerId"), static_cast<int>(interaction.getPlayerId()));
    map.insert(QStringLiteral("fee"), static_cast<qlonglong>(interaction.getFee()));

    const League* sellerLeague = resolveLeague(interaction.getSellerLeagueId());
    const League* buyerLeague = resolveLeague(interaction.getBuyerLeagueId());
    map.insert(QStringLiteral("sellerTeamName"),
        sellerLeague ? fromStd(sellerLeague->getTeamName(interaction.getSellerTeamId())) : QString());
    map.insert(QStringLiteral("buyerTeamName"),
        buyerLeague ? fromStd(buyerLeague->getTeamName(interaction.getBuyerTeamId())) : QString());

    QString playerName;
    if (sellerLeague) {
        if (const Footballer* player = sellerLeague->findPlayerById(interaction.getPlayerId())) {
            playerName = fromStd(player->getName());
        }
    }
    map.insert(QStringLiteral("playerName"), playerName);

    return map;
}

QVariantMap GameFacade::toPendingTransferOfferMap(const TransferOffer& offer) const {
    QVariantMap map;
    map.insert(QStringLiteral("offerId"), static_cast<int>(offer.id));
    map.insert(QStringLiteral("sellerLeagueId"), static_cast<int>(offer.sellerLeagueId));
    map.insert(QStringLiteral("sellerTeamId"), static_cast<int>(offer.sellerTeamId));
    map.insert(QStringLiteral("buyerLeagueId"), static_cast<int>(offer.buyerLeagueId));
    map.insert(QStringLiteral("buyerTeamId"), static_cast<int>(offer.buyerTeamId));
    map.insert(QStringLiteral("playerId"), static_cast<int>(offer.playerId));
    map.insert(QStringLiteral("fee"), static_cast<qlonglong>(offer.fee));
    map.insert(QStringLiteral("createdAtText"), formatDate(offer.createdAt));
    map.insert(QStringLiteral("lastDateText"), formatDate(offer.lastValidDate));
    map.insert(QStringLiteral("expiryPolicyText"), formatTransferOfferExpiryPolicy(offer.expiryPolicy));

    const League* sellerLeague = resolveLeague(offer.sellerLeagueId);
    const League* buyerLeague = resolveLeague(offer.buyerLeagueId);

    map.insert(QStringLiteral("sellerTeamName"),
        sellerLeague ? fromStd(sellerLeague->getTeamName(offer.sellerTeamId)) : QString());
    map.insert(QStringLiteral("buyerTeamName"),
        buyerLeague ? fromStd(buyerLeague->getTeamName(offer.buyerTeamId)) : QString());

    QString playerName;
    if (sellerLeague) {
        if (const Footballer* player = sellerLeague->findPlayerById(offer.playerId)) {
            playerName = fromStd(player->getName());
        }
    }
    map.insert(QStringLiteral("playerName"), playerName);

    return map;
}
