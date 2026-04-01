#include"GameFacade.h"
#include"Game.h"
#include"League.h"
#include"Team.h"
#include"Footballer.h"
#include"LeagueContext.h"

#include<QDebug>

#include<algorithm>
#include<exception>
#include<array>
#include<optional>
#include<string>
#include<vector>

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
    : QObject(parent) {
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

LeagueId GameFacade::resolveDefaultLeagueId() const {
    LeagueId resolvedLeagueId = 0;
    ensureGame()->forEachLeagueContext([&](const LeagueContext& context) {
        if (resolvedLeagueId == 0) {
            resolvedLeagueId = context.getLeague().getId();
        }
        });
    return resolvedLeagueId;
}

bool GameFacade::startNewGameInternal(LeagueId leagueId, TeamId teamId, const QString& newManagerName) {
    const QString trimmedManagerName = newManagerName.trimmed();
    if (leagueId == 0) {
        setLastError(QStringLiteral("Please choose a valid league."));
        qWarning() << "[GameFacade::startNewGame] Rejected invalid league id:" << leagueId;
        emit gameStateChanged();
        return false;
    }
    if (teamId == 0) {
        setLastError(QStringLiteral("Please choose a valid team."));
        qWarning() << "[GameFacade::startNewGame] Rejected invalid team id:" << teamId;
        emit gameStateChanged();
        return false;
    }
    if (trimmedManagerName.isEmpty()) {
        setLastError(QStringLiteral("Please enter a manager name."));
        qWarning() << "[GameFacade::startNewGame] Rejected empty manager name.";
        emit gameStateChanged();
        return false;
    }

    std::unique_ptr<Game> newGame = std::make_unique<Game>();
    LeagueContext* selectedContext = newGame->findLeagueContextById(leagueId);
    if (!selectedContext) {
        setLastError(QStringLiteral("Selected league could not be found."));
        qWarning() << "[GameFacade::startNewGame] League id is not present in fresh game:" << leagueId;
        emit gameStateChanged();
        return false;
    }

    Team* teamInNewGame = selectedContext->getLeague().findTeamById(teamId);
    if (!teamInNewGame) {
        setLastError(QStringLiteral("Selected team could not be found in the selected league."));
        qWarning() << "[GameFacade::startNewGame] Team id" << teamId << "not found in league" << leagueId;
        emit gameStateChanged();
        return false;
    }

    newGame->setUserTeam(teamInNewGame->getName());
    game = std::move(newGame);
    selectedLeagueId = leagueId;
    selectedTeamId = teamInNewGame->getId();
    managerName = trimmedManagerName;
    gameStarted = true;
    setLastError(QString());
    qDebug() << "[GameFacade::startNewGame] Started new game with league" << selectedLeagueId << "team id" << selectedTeamId;
    emit gameStateChanged();
    return true;
}

void GameFacade::setLastError(const QString& errorMessage) {
    lastError = errorMessage;
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
        for (const auto& [teamName, team] : league.getTeams()) {
            (void)teamName;
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
        LeagueId leagueId = selectedLeagueId;
        if (leagueId == 0) {
            leagueId = resolveDefaultLeagueId();
        }
        if (leagueId != 0) {
            const League* selectedLeague = resolveLeague(leagueId);
            if (!selectedLeague || !selectedLeague->hasTeam(static_cast<TeamId>(teamId))) {
                leagueId = 0;
            }
        }
 
        if (leagueId == 0) {
            ensureGame()->forEachLeagueContext([&](const LeagueContext& context) {
                if (leagueId != 0) {
                    return;
                }
                if (context.getLeague().hasTeam(static_cast<TeamId>(teamId))) {
                    leagueId = context.getLeague().getId();
                }
                });
        }
        return startNewGameInternal(leagueId, static_cast<TeamId>(teamId), newManagerName);
    }
    catch (const std::exception& ex) {
        qWarning() << "[GameFacade::startNewGame] Exception:" << ex.what();
        if (!gameStarted) {
            selectedLeagueId = 0;
            selectedTeamId = 0;
            managerName.clear();
        }
        setLastError(QStringLiteral("Failed to start a new game. Please try again."));
        emit gameStateChanged();
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
        emit gameStateChanged();
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
        emit gameStateChanged();
        return false;
    }
    catch (...) {
        qWarning() << "[GameFacade::startNewGameForLeagueTeam] Unknown exception";
        setLastError(QStringLiteral("Failed to start a new game. Please try again."));
        emit gameStateChanged();
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
    if (lastError.isEmpty()) {
        return;
    }

    lastError.clear();
    emit gameStateChanged();
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
            dashboard.insert(QStringLiteral("standingsRow"),
                toStandingsRowMap(standings[index], static_cast<int>(index + 1)));
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

    emit gameStateChanged();
    return true;
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
        QVariantMap row = toStandingsRowMap(standings[index], static_cast<int>(index + 1));
        row.insert(QStringLiteral("recentForm"), fromStd(league->getRecentFormString(standings[index].teamId, 5)));
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

QVariantMap GameFacade::toStandingsRowMap(const StandingsEntry& entry, int position) const {
    QVariantMap map;
    const League* league = resolveLeague(selectedLeagueId);
    map.insert(QStringLiteral("position"), position);
    map.insert(QStringLiteral("teamId"), static_cast<int>(entry.teamId));
    map.insert(QStringLiteral("teamName"), league ? fromStd(league->getTeamName(entry.teamId)) : QString());
    map.insert(QStringLiteral("played"), entry.played);
    map.insert(QStringLiteral("wins"), entry.wins);
    map.insert(QStringLiteral("draws"), entry.draws);
    map.insert(QStringLiteral("losses"), entry.losses);
    map.insert(QStringLiteral("goalsFor"), entry.goalsFor);
    map.insert(QStringLiteral("goalsAgainst"), entry.goalsAgainst);
    map.insert(QStringLiteral("goalDifference"), entry.goalDifference);
    map.insert(QStringLiteral("points"), entry.points);
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
        map.insert(QStringLiteral("teamName"), fromStd(player.getTeam()));
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