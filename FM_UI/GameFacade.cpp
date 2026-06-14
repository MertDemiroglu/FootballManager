#include"GameFacade.h"

#include"BootstrapPaths.h"
#include"SaveSlotService.h"

#include"fm/core/Game.h"
#include"fm/core/LeagueContext.h"
#include"fm/data/BootstrapDtos.h"
#include"fm/data/RuntimeSaveValidator.h"
#include"fm/data/SavePolicy.h"
#include"fm/data/SqliteBootstrapDatabaseInitializer.h"
#include"fm/data/SqliteBootstrapRepository.h"
#include"fm/roster/Team.h"
#include"fm/roster/Footballer.h"
#include"fm/roster/Formation.h"
#include"fm/roster/FormationPitchLayout.h"
#include"fm/roster/FormationSlotRole.h"
#include"fm/interaction/GameInteraction.h"
#include"fm/interaction/PostMatchInteraction.h"
#include"fm/interaction/PreMatchInteraction.h"
#include"fm/interaction/TransferOfferDecisionInteraction.h"
#include"fm/match/TeamSheet.h"
#include"fm/match/EditableLineup.h"
#include"fm/match/TeamSelectionService.h"
#include"fm/match/MatchReport.h"
#include"fm/presentation/MatchPresentationBuilder.h"
#include"fm/presentation/PresentationDtos.h"
#include"fm/presentation/TeamPresentationBuilder.h"
#include"fm/presentation/TeamSheetPresentationBuilder.h"
#include"fm/competition/League.h"
#include"fm/transfer/TransferOffer.h"
#include"fm/roster/PlayerPosition.h"

#include"StandingsTableModel.h"
#include"TeamPlayersModel.h"
#include"TeamRecentMatchesModel.h"
#include"TeamUpcomingMatchesModel.h"
#include"PendingTransferOffersModel.h"
#include"DashboardStateObject.h"
#include"DashboardUpcomingMatchesModel.h"
#include"InteractionStateObject.h"
#include"ShellStateObject.h"
#include"EditableLineupStateObject.h"
#include"EditableLineupSlotsModel.h"
#include"EditableLineupRosterModel.h"

#include<QDebug>
#include<QTemporaryDir>

#include<algorithm>
#include<cctype>
#include<cmath>
#include<cstdlib>
#include<exception>
#include<array>
#include<iomanip>
#include<unordered_map>
#include<optional>
#include<sstream>
#include<string>
#include<vector>
#include<utility>

namespace {
    QVariantMap missingPlayerMap() {
        QVariantMap map;
        map.insert(QStringLiteral("hasPlayer"), false);
        return map;
    }

    bool warnedUnsupportedEditableLineupPitchLayout = false;

    QString fromStd(const std::string& value) {
        return QString::fromStdString(value);
    }

    Date initialGameDate() {
        return Date(2025, Month::July, 1);
    }

    bool sameDate(const Date& lhs, const Date& rhs) {
        return lhs.getYear() == rhs.getYear()
            && lhs.getMonth() == rhs.getMonth()
            && lhs.getDay() == rhs.getDay();
    }

    void requireNewSaveReadyToCommit(Game& game, const GameBootstrapOptions& options) {
        const Date expectedInitialDate = initialGameDate();

        if (!sameDate(game.getDate(), expectedInitialDate)) {
            throw std::runtime_error("new save did not start on the configured initial game date");
        }

        const SaveMetadata metadata = game.getSaveMetadata();
        const SaveValidationResult validation = RuntimeSaveValidator::validateNewSave(
            options.sqliteDbPath,
            metadata,
            expectedInitialDate);
        if (!validation.valid) {
            throw std::runtime_error(validation.reason);
        }
    }

    void requireLoadedSaveDateSource(const Game& game, const GameBootstrapOptions& options) {
        const SaveMetadata metadata = game.getSaveMetadata();
        const SaveValidationResult validation = RuntimeSaveValidator::validateExistingSave(
            options.sqliteDbPath,
            &metadata);
        if (!validation.valid) {
            throw std::runtime_error(validation.reason);
        }
        if (!sameDate(game.getDate(), validation.currentDate)) {
            throw std::runtime_error("loaded save game date does not match game_state current date");
        }
    }

    std::string trim(std::string value) {
        const auto isNotSpace = [](unsigned char ch) {
            return !std::isspace(ch);
        };

        while (!value.empty() && !isNotSpace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && !isNotSpace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
        return value;
    }

    bool parseHexPair(const std::string& value, std::size_t offset, int& result) {
        if (offset + 2 > value.size()) {
            return false;
        }

        const std::string token = value.substr(offset, 2);
        char* end = nullptr;
        const long parsed = std::strtol(token.c_str(), &end, 16);
        if (!end || *end != '\0' || parsed < 0 || parsed > 255) {
            return false;
        }

        result = static_cast<int>(parsed);
        return true;
    }

    QString readableTextColorForPrimary(const std::string& primaryColor) {
        const std::string value = trim(primaryColor);
        if (value.size() != 7 || value.front() != '#') {
            return QStringLiteral("#f8fafc");
        }

        int red = 0;
        int green = 0;
        int blue = 0;
        if (!parseHexPair(value, 1, red) || !parseHexPair(value, 3, green) || !parseHexPair(value, 5, blue)) {
            return QStringLiteral("#f8fafc");
        }

        const int luminance = static_cast<int>((0.2126 * red) + (0.7152 * green) + (0.0722 * blue));
        return luminance >= 110 ? QStringLiteral("#071016") : QStringLiteral("#f8fafc");
    }

    int seedPlayerPower(const PlayerSeedData& player) {
        if (player.position == "Goalkeeper") {
            return (player.s1 * 3 + player.s2 * 2 + player.s3 * 2 + player.s4 + player.s5) / 9;
        }
        if (player.position.find("Midfielder") != std::string::npos) {
            return (player.s1 * 3 + player.s2 * 2 + player.s3 * 2 + player.s4 + player.s5 * 2) / 10;
        }
        if (player.position.find("Winger") != std::string::npos || player.position == "Striker") {
            return (player.s1 * 3 + player.s2 * 2 + player.s3 * 2 + player.s4 + player.s5) / 9;
        }
        return (player.s1 * 3 + player.s2 * 2 + player.s3 * 2 + player.s4 + player.s5) / 9;
    }

    int seedTeamRating(const TeamSeedData& team) {
        if (team.players.empty()) {
            return 0;
        }

        int total = 0;
        for (const PlayerSeedData& player : team.players) {
            total += seedPlayerPower(player);
        }
        return total / static_cast<int>(team.players.size());
    }

    TeamVisualDto teamVisualForTeam(const Team* team) {
        return TeamPresentationBuilder{}.buildTeamVisual(team);
    }

    QVariantMap toVariantMap(const TeamVisualDto& team) {
        QVariantMap map;
        map.insert(QStringLiteral("teamId"), static_cast<int>(team.teamId));
        map.insert(QStringLiteral("name"), fromStd(team.name));
        map.insert(QStringLiteral("primaryColor"), fromStd(team.primaryColor));
        map.insert(QStringLiteral("secondaryColor"), fromStd(team.secondaryColor));
        map.insert(QStringLiteral("textColor"), fromStd(team.textColor));
        return map;
    }

    QVariantMap toVariantMap(const SaveSlotInfo& info) {
        QVariantMap map;
        map.insert(QStringLiteral("saveSlotId"), info.saveSlotId);
        map.insert(QStringLiteral("saveName"), info.saveName);
        map.insert(QStringLiteral("managerName"), info.managerName);
        map.insert(QStringLiteral("managedLeagueId"), info.managedLeagueId);
        map.insert(QStringLiteral("managedTeamId"), info.managedTeamId);
        map.insert(QStringLiteral("managedTeamName"), info.managedTeamName);
        map.insert(QStringLiteral("managedLeagueName"), info.managedLeagueName);
        map.insert(QStringLiteral("currentDate"), info.currentDate);
        map.insert(QStringLiteral("createdAtUtc"), info.createdAtUtc);
        map.insert(QStringLiteral("updatedAtUtc"), info.updatedAtUtc);
        map.insert(QStringLiteral("schemaVersion"), info.schemaVersion);
        map.insert(QStringLiteral("worldVersion"), info.worldVersion);
        map.insert(QStringLiteral("isValid"), info.isValid);
        return map;
    }

    QVariantMap makeTeamVisualMap(const Team* team) {
        return toVariantMap(teamVisualForTeam(team));
    }

    void insertTeamVisualFields(QVariantMap& map, const QString& prefix, const Team* team) {
        const TeamVisualDto visual = teamVisualForTeam(team);
        map.insert(prefix + QStringLiteral("PrimaryColor"), fromStd(visual.primaryColor));
        map.insert(prefix + QStringLiteral("SecondaryColor"), fromStd(visual.secondaryColor));
        map.insert(prefix + QStringLiteral("TextColor"), fromStd(visual.textColor));
    }

    QString primaryColorForTeam(const Team* team) {
        return fromStd(teamVisualForTeam(team).primaryColor);
    }

    QString secondaryColorForTeam(const Team* team) {
        return fromStd(teamVisualForTeam(team).secondaryColor);
    }

    QString badgeTextColorForTeam(const Team* team) {
        return fromStd(teamVisualForTeam(team).textColor);
    }

    QVariantMap toVariantMap(const LineupSummaryDto& summary) {
        QVariantMap map;
        map.insert(QStringLiteral("assignedPlayers"), summary.assignedPlayers);
        if (summary.averageOverall.has_value()) {
            map.insert(QStringLiteral("averageOverall"), *summary.averageOverall);
        } else {
            map.insert(QStringLiteral("averageOverall"), QVariant());
        }
        map.insert(QStringLiteral("averageOverallText"), fromStd(summary.averageOverallText));
        return map;
    }

    QVariantMap toVariantMap(const LineupPlayerViewDto& player, bool isManagedTeam) {
        QVariantMap row;
        row.insert(QStringLiteral("slotIndex"), player.slotIndex);
        row.insert(QStringLiteral("slotRole"), fromStd(player.slotRoleText));
        row.insert(QStringLiteral("slotRoleKey"), player.slotRoleKey);
        row.insert(QStringLiteral("slotRoleText"), fromStd(player.slotRoleText));
        row.insert(QStringLiteral("roleText"), fromStd(player.roleText));
        row.insert(QStringLiteral("playerId"), static_cast<int>(player.playerId));
        row.insert(QStringLiteral("hasPlayer"), player.hasPlayer);
        row.insert(QStringLiteral("isManagedTeam"), isManagedTeam);
        row.insert(QStringLiteral("pitchX"), player.pitchX);
        row.insert(QStringLiteral("pitchY"), player.pitchY);
        row.insert(QStringLiteral("displayOrder"), player.displayOrder);
        row.insert(QStringLiteral("playerName"), fromStd(player.playerName));
        row.insert(QStringLiteral("surname"), fromStd(player.surname));
        row.insert(QStringLiteral("positionText"), fromStd(player.positionText));
        row.insert(QStringLiteral("overall"), player.overall);
        if (player.matchRating.has_value()) {
            row.insert(QStringLiteral("matchRating"), *player.matchRating);
        }
        return row;
    }

    QVariantList toLineupVariantList(const TeamSheetViewDto& sheet, TeamId selectedTeamId) {
        QVariantList rows;
        rows.reserve(static_cast<qsizetype>(sheet.players.size()));
        const bool isManagedTeam = sheet.team.teamId == selectedTeamId;
        for (const LineupPlayerViewDto& player : sheet.players) {
            rows.push_back(toVariantMap(player, isManagedTeam));
        }
        return rows;
    }

    QVariantMap toVariantMap(const TeamSheetViewDto& sheet, TeamId selectedTeamId) {
        QVariantMap map;
        map.insert(QStringLiteral("team"), toVariantMap(sheet.team));
        map.insert(QStringLiteral("coachName"), fromStd(sheet.coachName));
        map.insert(QStringLiteral("formationText"), fromStd(sheet.formationText));
        map.insert(QStringLiteral("summary"), toVariantMap(sheet.summary));
        map.insert(QStringLiteral("lineup"), toLineupVariantList(sheet, selectedTeamId));
        return map;
    }

    void insertTeamSheetFields(
        QVariantMap& map,
        const QString& prefix,
        const TeamSheetViewDto& sheet,
        TeamId selectedTeamId) {
        map.insert(prefix + QStringLiteral("Team"), toVariantMap(sheet.team));
        map.insert(prefix + QStringLiteral("TeamName"), fromStd(sheet.team.name));
        map.insert(prefix + QStringLiteral("PrimaryColor"), fromStd(sheet.team.primaryColor));
        map.insert(prefix + QStringLiteral("SecondaryColor"), fromStd(sheet.team.secondaryColor));
        map.insert(prefix + QStringLiteral("TextColor"), fromStd(sheet.team.textColor));
        map.insert(prefix + QStringLiteral("CoachName"), fromStd(sheet.coachName));
        map.insert(prefix + QStringLiteral("FormationText"), fromStd(sheet.formationText));
        map.insert(prefix + QStringLiteral("Lineup"), toLineupVariantList(sheet, selectedTeamId));
        map.insert(prefix + QStringLiteral("LineupSummary"), toVariantMap(sheet.summary));
        map.insert(prefix + QStringLiteral("AverageOverallText"), fromStd(sheet.summary.averageOverallText));
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

    QString joinSummaryLines(const QStringList& lines, const QString& emptyText) {
        return lines.isEmpty() ? emptyText : lines.join(QStringLiteral(", "));
    }

    const Team* resolveTeamForEvent(const Team* homeTeam,
        const Team* awayTeam,
        TeamId reportHomeId,
        TeamId reportAwayId,
        TeamId eventTeamId) {
        if (eventTeamId == reportHomeId) {
            return homeTeam;
        }
        if (eventTeamId == reportAwayId) {
            return awayTeam;
        }
        return nullptr;
    }

    QString resolveMatchPlayerName(const Team* homeTeam,
        const Team* awayTeam,
        TeamId reportHomeId,
        TeamId reportAwayId,
        TeamId eventTeamId,
        PlayerId playerId) {
        if (playerId == 0) {
            return QString();
        }

        if (const Team* teamForEvent = resolveTeamForEvent(homeTeam, awayTeam, reportHomeId, reportAwayId, eventTeamId)) {
            if (const Footballer* player = teamForEvent->findPlayerById(playerId)) {
                return fromStd(player->getName());
            }
        }

        if (homeTeam) {
            if (const Footballer* player = homeTeam->findPlayerById(playerId)) {
                return fromStd(player->getName());
            }
        }
        if (awayTeam) {
            if (const Footballer* player = awayTeam->findPlayerById(playerId)) {
                return fromStd(player->getName());
            }
        }

        return QStringLiteral("Unknown");
    }

    QString formatScorerSummary(const MatchReport& report, const Team* homeTeam, const Team* awayTeam) {
        std::unordered_map<PlayerId, int> goalsByPlayer;
        std::unordered_map<PlayerId, TeamId> playerTeamById;
        for (const MatchEventRecord& event : report.events) {
            if (event.kind != MatchEventKind::Goal || event.primaryPlayerId == 0) {
                continue;
            }
            ++goalsByPlayer[event.primaryPlayerId];
            playerTeamById[event.primaryPlayerId] = event.teamId;
        }

        QStringList lines;
        lines.reserve(static_cast<qsizetype>(goalsByPlayer.size()));
        for (const auto& [playerId, goals] : goalsByPlayer) {
            const TeamId teamId = playerTeamById[playerId];
            const QString playerName = resolveMatchPlayerName(
                homeTeam,
                awayTeam,
                report.homeId,
                report.awayId,
                teamId,
                playerId);
            lines.push_back(QStringLiteral("%1 x%2").arg(playerName).arg(goals));
        }
        lines.sort();
        return joinSummaryLines(lines, QStringLiteral("No goals recorded."));
    }

    QString formatAssistSummary(const MatchReport& report, const Team* homeTeam, const Team* awayTeam) {
        std::unordered_map<PlayerId, int> assistsByPlayer;
        std::unordered_map<PlayerId, TeamId> playerTeamById;
        for (const MatchEventRecord& event : report.events) {
            if (event.kind != MatchEventKind::Goal || event.secondaryPlayerId == 0) {
                continue;
            }
            ++assistsByPlayer[event.secondaryPlayerId];
            playerTeamById[event.secondaryPlayerId] = event.teamId;
        }

        QStringList lines;
        lines.reserve(static_cast<qsizetype>(assistsByPlayer.size()));
        for (const auto& [playerId, assists] : assistsByPlayer) {
            const TeamId teamId = playerTeamById[playerId];
            const QString playerName = resolveMatchPlayerName(
                homeTeam,
                awayTeam,
                report.homeId,
                report.awayId,
                teamId,
                playerId);
            lines.push_back(QStringLiteral("%1 x%2").arg(playerName).arg(assists));
        }
        lines.sort();
        return joinSummaryLines(lines, QStringLiteral("No assists recorded."));
    }

    QString formatCardSummary(const MatchReport& report, const Team* homeTeam, const Team* awayTeam) {
        std::unordered_map<PlayerId, int> yellowByPlayer;
        std::unordered_map<PlayerId, int> redByPlayer;
        std::unordered_map<PlayerId, TeamId> playerTeamById;
        for (const MatchEventRecord& event : report.events) {
            if (event.primaryPlayerId == 0) {
                continue;
            }
            if (event.kind == MatchEventKind::YellowCard) {
                ++yellowByPlayer[event.primaryPlayerId];
                playerTeamById[event.primaryPlayerId] = event.teamId;
            } else if (event.kind == MatchEventKind::RedCard) {
                ++redByPlayer[event.primaryPlayerId];
                playerTeamById[event.primaryPlayerId] = event.teamId;
            }
        }

        QStringList lines;
        for (const auto& [playerId, count] : yellowByPlayer) {
            const TeamId teamId = playerTeamById[playerId];
            const QString playerName = resolveMatchPlayerName(
                homeTeam,
                awayTeam,
                report.homeId,
                report.awayId,
                teamId,
                playerId);
            lines.push_back(QStringLiteral("%1 %2Y").arg(playerName).arg(count));
        }
        for (const auto& [playerId, count] : redByPlayer) {
            const TeamId teamId = playerTeamById[playerId];
            const QString playerName = resolveMatchPlayerName(
                homeTeam,
                awayTeam,
                report.homeId,
                report.awayId,
                teamId,
                playerId);
            lines.push_back(QStringLiteral("%1 %2R").arg(playerName).arg(count));
        }
        lines.sort();
        return joinSummaryLines(lines, QStringLiteral("No cards recorded."));
    }

    int passAccuracyPercent(const MatchTeamReportStats& stats) {
        if (stats.passesAttempted <= 0) {
            return 0;
        }
        return static_cast<int>(
            std::round(
                static_cast<double>(stats.passesCompleted) * 100.0
                / static_cast<double>(stats.passesAttempted)));
    }

    QVariantMap buildStatRow(const QString& label, const QString& homeValue, const QString& awayValue) {
        QVariantMap row;
        row.insert(QStringLiteral("label"), label);
        row.insert(QStringLiteral("homeValue"), homeValue);
        row.insert(QStringLiteral("awayValue"), awayValue);
        return row;
    }

    QVariantList buildMatchStatRows(const MatchReport& report) {
        QVariantList rows;
        rows.push_back(buildStatRow(
            QStringLiteral("Possession"),
            QStringLiteral("%1%").arg(static_cast<int>(std::round(report.homeStats.possessionShare))),
            QStringLiteral("%1%").arg(static_cast<int>(std::round(report.awayStats.possessionShare)))));
        rows.push_back(buildStatRow(
            QStringLiteral("Expected Goals (xG)"),
            QString::number(report.homeStats.expectedGoals, 'f', 2),
            QString::number(report.awayStats.expectedGoals, 'f', 2)));
        rows.push_back(buildStatRow(
            QStringLiteral("Total Shots"),
            QString::number(report.homeStats.shots),
            QString::number(report.awayStats.shots)));
        rows.push_back(buildStatRow(
            QStringLiteral("Shots on Target"),
            QString::number(report.homeStats.shotsOnTarget),
            QString::number(report.awayStats.shotsOnTarget)));
        rows.push_back(buildStatRow(
            QStringLiteral("Passes"),
            QString::number(report.homeStats.passesCompleted)
                + QStringLiteral("/")
                + QString::number(report.homeStats.passesAttempted),
            QString::number(report.awayStats.passesCompleted)
                + QStringLiteral("/")
                + QString::number(report.awayStats.passesAttempted)));
        rows.push_back(buildStatRow(
            QStringLiteral("Pass Accuracy"),
            QStringLiteral("%1%").arg(passAccuracyPercent(report.homeStats)),
            QStringLiteral("%1%").arg(passAccuracyPercent(report.awayStats))));
        rows.push_back(buildStatRow(
            QStringLiteral("Fouls"),
            QString::number(report.homeStats.fouls),
            QString::number(report.awayStats.fouls)));
        rows.push_back(buildStatRow(
            QStringLiteral("Corners"),
            QString::number(report.homeStats.corners),
            QString::number(report.awayStats.corners)));
        return rows;
    }

    QString eventKindText(MatchEventKind kind) {
        switch (kind) {
        case MatchEventKind::Goal:
            return QStringLiteral("Goal");
        case MatchEventKind::YellowCard:
            return QStringLiteral("Yellow Card");
        case MatchEventKind::RedCard:
            return QStringLiteral("Red Card");
        default:
            return QStringLiteral("Event");
        }
    }
}

GameFacade::GameFacade(QObject* parent)
    : GameFacade(GameBootstrapOptions{}, parent) {
}

GameFacade::GameFacade(const GameBootstrapOptions& bootstrapOptions, QObject* parent)
    : QObject(parent),
      bootstrapOptions(bootstrapOptions),
      standingsModel(this),
      teamPlayersModel(this),
      teamRecentMatchesModel(this),
      teamUpcomingMatchesModel(this),
      pendingTransferOffersModel(this),
      currentTeamSeasonStatsObject(this),
      dashboardStateObject(this),
      dashboardUpcomingMatchesModel(this),
      shellStateObject(this),
      interactionStateObject(this),
      editableLineupStateObject(this),
      editableLineupSlotsModel(this),
      editableLineupRosterModel(this) {
}

GameFacade::~GameFacade() {}

Game* GameFacade::ensureGame() {
    if (!gameStarted) {
        return nullptr;
    }
    if (!game) {
        if (bootstrapOptions.sqliteDbPath.empty()) {
            qWarning() << "[GameFacade::ensureGame] No explicit save bootstrap is available.";
            return nullptr;
        }
        game = std::make_unique<Game>(bootstrapOptions);
    }
    return game.get();
}

const Game* GameFacade::ensureGame() const {
    return const_cast<GameFacade*>(this)->ensureGame();
}

bool GameFacade::ensureSelectedTeamContext() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        qWarning() << "[GameFacade::ensureSelectedTeamContext] Active game flag is set but Game is unavailable."
                   << "gameStarted=" << gameStarted
                   << "selectedLeagueId=" << selectedLeagueId
                   << "selectedTeamId=" << selectedTeamId
                   << "managedLeagueId=" << 0
                   << "managedTeamId=" << 0;
        return false;
    }

    const LeagueId managedLeagueId = currentGame->getManagedLeagueId();
    const TeamId managedTeamId = currentGame->getManagedTeamId();

    const auto isValidTeamContext = [this](LeagueId leagueId, TeamId teamId) {
        if (leagueId == 0 || teamId == 0) {
            return false;
        }
        const League* league = resolveLeague(leagueId);
        return league && league->hasTeam(teamId);
    };

    if (isValidTeamContext(selectedLeagueId, selectedTeamId)) {
        if (managedLeagueId != 0
            && managedTeamId != 0
            && (managedLeagueId != selectedLeagueId || managedTeamId != selectedTeamId)) {
            if (isValidTeamContext(managedLeagueId, managedTeamId)) {
                qWarning() << "[GameFacade::ensureSelectedTeamContext] Selected team context differed from managed team; using managed team."
                           << "gameStarted=" << gameStarted
                           << "selectedLeagueId=" << selectedLeagueId
                           << "selectedTeamId=" << selectedTeamId
                           << "managedLeagueId=" << managedLeagueId
                           << "managedTeamId=" << managedTeamId;
                selectedLeagueId = managedLeagueId;
                selectedTeamId = managedTeamId;
            } else {
                qWarning() << "[GameFacade::ensureSelectedTeamContext] Managed team context is invalid; keeping valid selected team context."
                           << "gameStarted=" << gameStarted
                           << "selectedLeagueId=" << selectedLeagueId
                           << "selectedTeamId=" << selectedTeamId
                           << "managedLeagueId=" << managedLeagueId
                           << "managedTeamId=" << managedTeamId;
            }
        }
        return true;
    }

    if (isValidTeamContext(managedLeagueId, managedTeamId)) {
        qWarning() << "[GameFacade::ensureSelectedTeamContext] Recovered selected team context from managed team."
                   << "gameStarted=" << gameStarted
                   << "selectedLeagueId=" << selectedLeagueId
                   << "selectedTeamId=" << selectedTeamId
                   << "managedLeagueId=" << managedLeagueId
                   << "managedTeamId=" << managedTeamId;
        selectedLeagueId = managedLeagueId;
        selectedTeamId = managedTeamId;
        return true;
    }

    qWarning() << "[GameFacade::ensureSelectedTeamContext] Unable to recover selected team context."
               << "gameStarted=" << gameStarted
               << "selectedLeagueId=" << selectedLeagueId
               << "selectedTeamId=" << selectedTeamId
               << "managedLeagueId=" << managedLeagueId
               << "managedTeamId=" << managedTeamId;
    return false;
}

bool GameFacade::hasValidSelectedTeam() {
    return ensureSelectedTeamContext();
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

    QString selectedTeamName;
    const QVariantList teams = getTeamSelectionList();
    for (const QVariant& value : teams) {
        const QVariantMap team = value.toMap();
        if (team.value(QStringLiteral("leagueId")).toInt() == static_cast<int>(leagueId)
            && team.value(QStringLiteral("teamId")).toInt() == static_cast<int>(teamId)) {
            selectedTeamName = team.value(QStringLiteral("teamName")).toString();
            break;
        }
    }

    if (selectedTeamName.isEmpty()) {
        setLastError(QStringLiteral("Selected team could not be found in the selected league."));
        qWarning() << "[GameFacade::startNewGame] Team id" << teamId << "not found in league" << leagueId;
        publishGameStateChanged();
        return false;
    }

    const QString saveName = trimmedManagerName + QStringLiteral(" - ") + selectedTeamName;
    return createNewGameSave(saveName, static_cast<int>(leagueId), static_cast<int>(teamId), trimmedManagerName);
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
        QString primaryColor;
        QString secondaryColor;
        QString textColor;
        int rating = 0;
        int squadSize = 0;
    };

    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        throw std::runtime_error("failed to create temporary database directory for team selection");
    }

    const QString previewDbPath = temporaryDirectory.filePath(QStringLiteral("team_selection_preview.db"));
    SqliteBootstrapDatabaseInitializer::resetWithSeed(
        previewDbPath.toStdString(),
        BootstrapPaths::schemaAssetPath().toStdString(),
        BootstrapPaths::seedAssetPath().toStdString());

    const SqliteBootstrapRepository repository(previewDbPath.toStdString());
    const std::vector<LeagueSeedData> leagues = repository.loadLeagues();
    std::vector<TeamSummary> summaries;
    for (const LeagueSeedData& league : leagues) {
        for (const TeamSeedData& team : league.teams) {
            const std::string primaryColor = trim(team.primaryColor).empty() ? "#22c55e" : team.primaryColor;
            const std::string secondaryColor = trim(team.secondaryColor).empty() ? "#0f172a" : team.secondaryColor;
            summaries.push_back(TeamSummary{
                league.id,
                team.id,
                fromStd(team.name),
                fromStd(league.name),
                fromStd(primaryColor),
                fromStd(secondaryColor),
                readableTextColorForPrimary(primaryColor),
                seedTeamRating(team),
                static_cast<int>(team.players.size())
                });
        }
    }
    

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
        item.insert(QStringLiteral("primaryColor"), summary.primaryColor);
        item.insert(QStringLiteral("secondaryColor"), summary.secondaryColor);
        item.insert(QStringLiteral("textColor"), summary.textColor);
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

bool GameFacade::hasContinueSave() const {
    const SaveSlotService service;
    const QString saveSlotId = service.lastSaveSlotId();
    return !saveSlotId.isEmpty() && service.saveSlotExists(saveSlotId);
}

QVariantList GameFacade::listSaveSlots() const {
    QVariantList result;
    const SaveSlotService service;
    const QList<SaveSlotInfo> saveSlots = service.listSaveSlots();
    result.reserve(saveSlots.size());
    for (const SaveSlotInfo& saveSlot : saveSlots) {
        result.push_back(toVariantMap(saveSlot));
    }
    return result;
}

bool GameFacade::createNewGameSave(
    const QString& saveName,
    int leagueId,
    int teamId,
    const QString& newManagerName) {
    const QString trimmedSaveName = saveName.trimmed();
    const QString trimmedManagerName = newManagerName.trimmed();
    if (trimmedSaveName.isEmpty()) {
        setLastError(QStringLiteral("Please enter a save name."));
        publishGameStateChanged();
        return false;
    }
    if (trimmedManagerName.isEmpty()) {
        setLastError(QStringLiteral("Please enter a manager name."));
        publishGameStateChanged();
        return false;
    }
    if (leagueId <= 0 || teamId <= 0) {
        setLastError(QStringLiteral("Please choose a valid league and team."));
        publishGameStateChanged();
        return false;
    }

    QString createdSaveSlotId;
    try {
        const SaveSlotService service;
        createdSaveSlotId = service.createUniqueSaveSlotId(trimmedSaveName);
        GameBootstrapOptions newBootstrapOptions = service.createNewSaveBootstrapOptions(createdSaveSlotId, trimmedSaveName);
        auto newGame = std::make_unique<Game>(newBootstrapOptions);

        LeagueContext* selectedContext = newGame->findLeagueContextById(static_cast<LeagueId>(leagueId));
        if (!selectedContext) {
            QString deleteError;
            service.deleteSaveSlot(createdSaveSlotId, &deleteError);
            setLastError(QStringLiteral("Selected league could not be found."));
            publishGameStateChanged();
            return false;
        }
        Team* selectedTeam = selectedContext->getLeague().findTeamById(static_cast<TeamId>(teamId));
        if (!selectedTeam) {
            QString deleteError;
            service.deleteSaveSlot(createdSaveSlotId, &deleteError);
            setLastError(QStringLiteral("Selected team could not be found in the selected league."));
            publishGameStateChanged();
            return false;
        }

        newGame->setUserTeam(static_cast<LeagueId>(leagueId), selectedTeam->getId());
        newGame->setSaveManagerName(trimmedManagerName.toStdString());
        newGame->pauseSimulation();
        newGame->flushSaveState();
        requireNewSaveReadyToCommit(*newGame, newBootstrapOptions);

        bootstrapOptions = std::move(newBootstrapOptions);
        game = std::move(newGame);
        selectedLeagueId = static_cast<LeagueId>(leagueId);
        selectedTeamId = selectedTeam->getId();
        managerName = trimmedManagerName;
        currentSaveSlotId = createdSaveSlotId;
        gameStarted = true;
        const QString committedSaveSlotId = createdSaveSlotId;
        createdSaveSlotId.clear();
        service.rememberLastSaveSlot(committedSaveSlotId);
        clearEditableLineupData();
        setLastError(QString());
        publishGameStateChanged();
        return true;
    } catch (const std::exception& ex) {
        qWarning() << "[GameFacade::createNewGameSave] Exception:" << ex.what();
        if (!createdSaveSlotId.isEmpty()) {
            QString deleteError;
            SaveSlotService{}.deleteSaveSlot(createdSaveSlotId, &deleteError);
        }
        setLastError(QStringLiteral("Failed to create a valid new save: %1").arg(QString::fromLocal8Bit(ex.what())));
        publishGameStateChanged();
        return false;
    } catch (...) {
        qWarning() << "[GameFacade::createNewGameSave] Unknown exception";
        if (!createdSaveSlotId.isEmpty()) {
            QString deleteError;
            SaveSlotService{}.deleteSaveSlot(createdSaveSlotId, &deleteError);
        }
        setLastError(QStringLiteral("Failed to create a new save."));
        publishGameStateChanged();
        return false;
    }
}

bool GameFacade::loadGameSave(const QString& saveSlotId) {
    const QString trimmedSaveSlotId = saveSlotId.trimmed();
    try {
        const SaveSlotService service;
        GameBootstrapOptions loadedBootstrapOptions = service.loadExistingSaveBootstrapOptions(trimmedSaveSlotId);
        auto loadedGame = std::make_unique<Game>(loadedBootstrapOptions);
        const SaveMetadata metadata = loadedGame->getSaveMetadata();
        requireLoadedSaveDateSource(*loadedGame, loadedBootstrapOptions);
        // TODO: Transfer offers and editable lineup drafts need their own runtime
        // persistence phase; date, fixtures/results, standings, and player condition
        // are restored by the core runtime state repository.

        LeagueContext* selectedContext = loadedGame->findLeagueContextById(metadata.managedLeagueId);
        Team* selectedTeam = selectedContext
            ? selectedContext->getLeague().findTeamById(metadata.managedTeamId)
            : nullptr;
        if (!selectedContext || !selectedTeam) {
            setLastError(QStringLiteral("Save metadata points to a managed club that no longer exists."));
            publishGameStateChanged();
            return false;
        }

        loadedGame->setUserTeam(metadata.managedLeagueId, metadata.managedTeamId);
        loadedGame->setSaveManagerName(metadata.managerName);
        loadedGame->pauseSimulation();

        bootstrapOptions = std::move(loadedBootstrapOptions);
        game = std::move(loadedGame);
        selectedLeagueId = metadata.managedLeagueId;
        selectedTeamId = metadata.managedTeamId;
        managerName = fromStd(metadata.managerName);
        currentSaveSlotId = trimmedSaveSlotId;
        gameStarted = true;
        service.rememberLastSaveSlot(trimmedSaveSlotId);
        clearEditableLineupData();
        setLastError(QString());
        publishGameStateChanged();
        return true;
    } catch (const std::exception& ex) {
        qWarning() << "[GameFacade::loadGameSave] Exception:" << ex.what();
        setLastError(QStringLiteral("Failed to load save."));
        publishGameStateChanged();
        return false;
    } catch (...) {
        qWarning() << "[GameFacade::loadGameSave] Unknown exception";
        setLastError(QStringLiteral("Failed to load save."));
        publishGameStateChanged();
        return false;
    }
}

bool GameFacade::continueLastSave() {
    const SaveSlotService service;
    const QString saveSlotId = service.lastSaveSlotId();
    if (saveSlotId.isEmpty()) {
        setLastError(QStringLiteral("No save is available to continue."));
        publishGameStateChanged();
        return false;
    }
    return loadGameSave(saveSlotId);
}

bool GameFacade::deleteGameSave(const QString& saveSlotId) {
    const QString trimmedSaveSlotId = saveSlotId.trimmed();
    if (game && trimmedSaveSlotId == currentSaveSlotId) {
        setLastError(QStringLiteral("Cannot delete the currently loaded save."));
        publishGameStateChanged();
        return false;
    }

    const SaveSlotService service;
    QString errorMessage;
    if (!service.deleteSaveSlot(trimmedSaveSlotId, &errorMessage)) {
        setLastError(errorMessage.isEmpty() ? QStringLiteral("Failed to delete save.") : errorMessage);
        publishGameStateChanged();
        return false;
    }

    setLastError(QString());
    publishGameStateChanged();
    return true;
}

QString GameFacade::getCurrentSaveSlotId() const {
    return currentSaveSlotId;
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
    const_cast<GameFacade*>(this)->ensureSelectedTeamContext();
    if (!hasValidSelectedTeam()) {
        return QString();
    }
    const League* league = resolveLeague(selectedLeagueId);
    return league ? fromStd(league->getTeamName(selectedTeamId)) : QString();
}

int GameFacade::getSelectedLeagueId() const {
    const_cast<GameFacade*>(this)->ensureSelectedTeamContext();
    return static_cast<int>(selectedLeagueId);
}

int GameFacade::getSelectedTeamId() const {
    const_cast<GameFacade*>(this)->ensureSelectedTeamContext();
    return static_cast<int>(selectedTeamId);
}

QString GameFacade::getManagerName() const {
    return managerName;
}

QString GameFacade::getLastError() const {
    return lastError;
}

QVariantMap GameFacade::getCurrentSaveMetadata() const {
    const Game* currentGame = ensureGame();
    const SaveMetadata metadata = currentGame ? currentGame->getSaveMetadata() : SaveMetadata{};

    QVariantMap map;
    map.insert(QStringLiteral("saveSlotId"), fromStd(metadata.saveSlotId));
    map.insert(QStringLiteral("saveName"), fromStd(metadata.saveName));
    map.insert(QStringLiteral("managerName"), fromStd(metadata.managerName));
    map.insert(QStringLiteral("managedLeagueId"), static_cast<int>(metadata.managedLeagueId));
    map.insert(QStringLiteral("managedTeamId"), static_cast<int>(metadata.managedTeamId));
    map.insert(QStringLiteral("currentDate"), fromStd(metadata.currentDate));
    map.insert(QStringLiteral("createdAtUtc"), fromStd(metadata.createdAtUtc));
    map.insert(QStringLiteral("updatedAtUtc"), fromStd(metadata.updatedAtUtc));
    map.insert(QStringLiteral("schemaVersion"), metadata.schemaVersion);
    map.insert(QStringLiteral("worldVersion"), metadata.worldVersion);
    return map;
}

bool GameFacade::saveGame() {
    if (!gameStarted) {
        setLastError(QStringLiteral("No active game is available to save."));
        qWarning() << "[GameFacade::saveGame] No active game.";
        publishGameStateChanged();
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame || !currentGame->manualSave()) {
        setLastError(QStringLiteral("Failed to save game."));
        qWarning() << "[GameFacade::saveGame] Manual save failed.";
        publishGameStateChanged();
        return false;
    }

    setLastError(QString());
    publishGameStateChanged();
    return true;
}

QVariantList GameFacade::getAutoSaveFrequencyOptions() const {
    const std::array<AutoSaveFrequency, 5> frequencies{
        AutoSaveFrequency::ManualOnly,
        AutoSaveFrequency::Daily,
        AutoSaveFrequency::Every3Days,
        AutoSaveFrequency::Weekly,
        AutoSaveFrequency::Every2Weeks
    };

    QVariantList result;
    result.reserve(static_cast<qsizetype>(frequencies.size()));
    for (AutoSaveFrequency frequency : frequencies) {
        QVariantMap option;
        option.insert(QStringLiteral("stableCode"), fromStd(std::string(toStableCode(frequency))));
        option.insert(QStringLiteral("displayText"), fromStd(std::string(toDisplayText(frequency))));
        result.push_back(option);
    }
    return result;
}

QString GameFacade::getAutoSaveFrequencyCode() const {
    const Game* currentGame = ensureGame();
    const AutoSaveFrequency frequency = currentGame
        ? currentGame->getAutoSaveFrequency()
        : AutoSaveFrequency::Weekly;
    return fromStd(std::string(toStableCode(frequency)));
}

bool GameFacade::setAutoSaveFrequency(const QString& stableCode) {
    if (!gameStarted) {
        setLastError(QStringLiteral("No active game is available for save settings."));
        publishGameStateChanged();
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        setLastError(QStringLiteral("No active game is available for save settings."));
        publishGameStateChanged();
        return false;
    }

    const std::optional<AutoSaveFrequency> frequency =
        autoSaveFrequencyFromStableCode(stableCode.toStdString());
    if (!frequency.has_value()) {
        setLastError(QStringLiteral("Invalid auto save frequency."));
        publishGameStateChanged();
        return false;
    }

    currentGame->setAutoSaveFrequency(*frequency);
    setLastError(QString());
    publishGameStateChanged();
    return true;
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

EditableLineupStateObject* GameFacade::getEditableLineupState() const {
    return const_cast<EditableLineupStateObject*>(&editableLineupStateObject);
}

EditableLineupSlotsModel* GameFacade::getEditableLineupSlotsModel() const {
    return const_cast<EditableLineupSlotsModel*>(&editableLineupSlotsModel);
}

EditableLineupRosterModel* GameFacade::getEditableLineupRosterModel() const {
    return const_cast<EditableLineupRosterModel*>(&editableLineupRosterModel);
}

EditableLineupRosterModel* GameFacade::getEditableLineupSubstitutesModel() const {
    return const_cast<EditableLineupRosterModel*>(&editableLineupSubstitutesModel);
}

QVariantMap GameFacade::getDashboard() const {
    QVariantMap dashboard;
    dashboard.insert(QStringLiteral("currentDateText"), getCurrentDateText());
    dashboard.insert(QStringLiteral("gameStateText"), getCurrentStateText());
    dashboard.insert(QStringLiteral("selectedTeamName"), getSelectedTeamName());
    dashboard.insert(QStringLiteral("selectedTeamId"), getSelectedTeamId());
    dashboard.insert(QStringLiteral("selectedTeamPrimaryColor"), primaryColorForTeam(nullptr));
    dashboard.insert(QStringLiteral("selectedTeamSecondaryColor"), secondaryColorForTeam(nullptr));
    dashboard.insert(QStringLiteral("selectedTeamTextColor"), badgeTextColorForTeam(nullptr));
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

    const Team* selectedTeam = league->findTeamById(selectedTeamId);
    insertTeamVisualFields(dashboard, QStringLiteral("selectedTeam"), selectedTeam);

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

    bool played = false;
    try {
        if (currentGame->getActivePreMatchInteraction()) {
            const bool applied = applyEditableLineupToActivePreMatch();
            if (!applied) {
                qWarning() << "[GameFacade::playActiveMatch] Editable lineup was not applied; using pending generated team sheet.";
            }
        }
        played = currentGame->playPendingPreMatch();
        if (!played) {
            setLastError(QStringLiteral("No playable pre-match interaction is active."));
        } else {
            setLastError(QString());
        }
    } catch (const std::exception& ex) {
        const QString errorMessage = QStringLiteral("Failed to play active match: %1").arg(QString::fromUtf8(ex.what()));
        qWarning() << "[GameFacade::playActiveMatch] Exception:" << errorMessage;
        setLastError(errorMessage);
        played = false;
    } catch (...) {
        qWarning() << "[GameFacade::playActiveMatch] Unknown exception";
        setLastError(QStringLiteral("Failed to play active match: unknown error."));
        played = false;
    }
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

bool GameFacade::ensureEditableLineupReady() {
    if (!ensureSelectedTeamContext()) {
        clearEditableLineupData();
        qWarning() << "[GameFacade::ensureEditableLineupReady] No valid selected team context for lineup editor."
                   << "gameStarted=" << gameStarted
                   << "selectedLeagueId=" << selectedLeagueId
                   << "selectedTeamId=" << selectedTeamId;
        return false;
    }

    refreshEditableLineup();

    const int expectedSlotCount = getExpectedEditableLineupSlotCount();
    const int selectedTeamPlayerCount = getSelectedTeamPlayerCount();
    const bool hasLineup = editableLineup.has_value();
    const bool stateHasLineup = editableLineupStateObject.hasLineup();
    const int slotCount = editableLineupSlotsModel.rowCount();
    const int rosterCount = editableLineupRosterModel.rowCount();
    bool ready = hasLineup
        && stateHasLineup
        && slotCount == expectedSlotCount
        && rosterCount == selectedTeamPlayerCount;

    if (!ready) {
        qWarning() << "[GameFacade::ensureEditableLineupReady] Lineup readiness failed before rebuild."
                   << "hasLineup=" << hasLineup
                   << "state.hasLineup=" << stateHasLineup
                   << "slots=" << slotCount
                   << "expectedSlots=" << expectedSlotCount
                   << "roster=" << rosterCount
                   << "expectedRoster=" << selectedTeamPlayerCount
                   << "selectedTeamId=" << selectedTeamId
                   << "selectedLeagueId=" << selectedLeagueId;
        refreshEditableLineup();
        ready = editableLineup.has_value()
            && editableLineupStateObject.hasLineup()
            && editableLineupSlotsModel.rowCount() == getExpectedEditableLineupSlotCount()
            && editableLineupRosterModel.rowCount() == getSelectedTeamPlayerCount();
    }
    if (!ready) {
        qWarning() << "[GameFacade::ensureEditableLineupReady] Lineup readiness failed after rebuild."
                   << "hasLineup=" << editableLineup.has_value()
                   << "state.hasLineup=" << editableLineupStateObject.hasLineup()
                   << "slots=" << editableLineupSlotsModel.rowCount()
                   << "expectedSlots=" << getExpectedEditableLineupSlotCount()
                   << "roster=" << editableLineupRosterModel.rowCount()
                   << "expectedRoster=" << getSelectedTeamPlayerCount()
                   << "selectedTeamId=" << selectedTeamId
                   << "selectedLeagueId=" << selectedLeagueId;
    }
    return ready;
}

QVariantList GameFacade::getEditableLineupSupportedFormations() const {
    QVariantList formations;

    for (FormationId formationId : getSupportedFormationIds()) {
        QVariantMap item;
        item.insert(QStringLiteral("formationId"), static_cast<int>(formationId));
        item.insert(QStringLiteral("formationText"), formatFormation(formationId));
        formations.push_back(item);
    }

    return formations;
}

QVariantList GameFacade::getEditableLineupSupportedMentalities() const {
    QVariantList mentalities;
    const std::array<TeamMentality, 3> supportedMentalities{
        TeamMentality::Defensive,
        TeamMentality::Balanced,
        TeamMentality::Attacking
    };

    for (TeamMentality mentality : supportedMentalities) {
        QVariantMap item;
        item.insert(QStringLiteral("stableCode"), fromStd(std::string(toStableCode(mentality))));
        item.insert(QStringLiteral("displayText"), fromStd(std::string(toDisplayText(mentality))));
        mentalities.push_back(item);
    }

    return mentalities;
}

QVariantList GameFacade::getEditableLineupSupportedTempos() const {
    QVariantList tempos;
    const std::array<TeamTempo, 3> supportedTempos{
        TeamTempo::Low,
        TeamTempo::Normal,
        TeamTempo::High
    };

    for (TeamTempo tempo : supportedTempos) {
        QVariantMap item;
        item.insert(QStringLiteral("stableCode"), fromStd(std::string(toStableCode(tempo))));
        item.insert(QStringLiteral("displayText"), fromStd(std::string(toDisplayText(tempo))));
        tempos.push_back(item);
    }

    return tempos;
}

QVariantMap GameFacade::getEditableLineupTacticalSetup() const {
    QVariantMap tacticalSetupMap;
    const EditableLineup* lineup = resolveEditableLineup();
    const TacticalSetup tacticalSetup = lineup ? lineup->getTacticalSetup() : TacticalSetup{};

    tacticalSetupMap.insert(QStringLiteral("mentalityCode"), fromStd(std::string(toStableCode(tacticalSetup.mentality))));
    tacticalSetupMap.insert(QStringLiteral("mentalityText"), fromStd(std::string(toDisplayText(tacticalSetup.mentality))));
    tacticalSetupMap.insert(QStringLiteral("tempoCode"), fromStd(std::string(toStableCode(tacticalSetup.tempo))));
    tacticalSetupMap.insert(QStringLiteral("tempoText"), fromStd(std::string(toDisplayText(tacticalSetup.tempo))));
    tacticalSetupMap.insert(QStringLiteral("widthCode"), fromStd(std::string(toStableCode(tacticalSetup.width))));
    tacticalSetupMap.insert(QStringLiteral("widthText"), fromStd(std::string(toDisplayText(tacticalSetup.width))));
    tacticalSetupMap.insert(QStringLiteral("defensiveLineCode"), fromStd(std::string(toStableCode(tacticalSetup.defensiveLine))));
    tacticalSetupMap.insert(QStringLiteral("defensiveLineText"), fromStd(std::string(toDisplayText(tacticalSetup.defensiveLine))));
    tacticalSetupMap.insert(QStringLiteral("pressingIntensityCode"), fromStd(std::string(toStableCode(tacticalSetup.pressingIntensity))));
    tacticalSetupMap.insert(QStringLiteral("pressingIntensityText"), fromStd(std::string(toDisplayText(tacticalSetup.pressingIntensity))));
    tacticalSetupMap.insert(QStringLiteral("markingStyleCode"), fromStd(std::string(toStableCode(tacticalSetup.markingStyle))));
    tacticalSetupMap.insert(QStringLiteral("markingStyleText"), fromStd(std::string(toDisplayText(tacticalSetup.markingStyle))));
    tacticalSetupMap.insert(QStringLiteral("passingDirectnessCode"), fromStd(std::string(toStableCode(tacticalSetup.passingDirectness))));
    tacticalSetupMap.insert(QStringLiteral("passingDirectnessText"), fromStd(std::string(toDisplayText(tacticalSetup.passingDirectness))));
    return tacticalSetupMap;
}

bool GameFacade::changeEditableLineupFormation(int formationId) {
    if (!gameStarted || formationId < 0) {
        return false;
    }

    const FormationId newFormationId = static_cast<FormationId>(formationId);
    if (!isFormationSupported(newFormationId)) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->changeFormation(newFormationId)) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::setEditableLineupMentality(const QString& stableCode) {
    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    const std::optional<TeamMentality> mentality = teamMentalityFromStableCode(stableCode.toStdString());
    if (!mentality.has_value()) {
        return false;
    }

    TacticalSetup tacticalSetup = lineup->getTacticalSetup();
    tacticalSetup.mentality = *mentality;
    lineup->setTacticalSetup(tacticalSetup);
    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    syncEditableLineupDisplayToActivePreMatch();
    emit gameStateChanged();
    return true;
}

bool GameFacade::setEditableLineupTempo(const QString& stableCode) {
    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    const std::optional<TeamTempo> tempo = teamTempoFromStableCode(stableCode.toStdString());
    if (!tempo.has_value()) {
        return false;
    }

    TacticalSetup tacticalSetup = lineup->getTacticalSetup();
    tacticalSetup.tempo = *tempo;
    lineup->setTacticalSetup(tacticalSetup);
    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    syncEditableLineupDisplayToActivePreMatch();
    emit gameStateChanged();
    return true;
}

bool GameFacade::autoSelectEditableLineup() {
    if (!gameStarted) {
        return false;
    }

    if (!ensureEditableLineupReady()) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    const League* league = resolveLeague(selectedLeagueId);
    const Team* team = league ? league->findTeamById(selectedTeamId) : nullptr;
    if (!team) {
        return false;
    }

    try {
        const TeamSelectionService teamSelectionService;
        TeamSheet teamSheet = teamSelectionService.buildTeamSheet(*team, lineup->getFormationId());
        teamSheet.tacticalSetup = lineup->getTacticalSetup();
        lineup->applyTeamSheet(teamSheet);
    }
    catch (const std::exception& ex) {
        qWarning() << "[GameFacade::autoSelectEditableLineup] Auto select failed:" << ex.what();
        return false;
    }
    catch (...) {
        qWarning() << "[GameFacade::autoSelectEditableLineup] Auto select failed with unknown error.";
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(true)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::assignEditableLineupPlayerToSlot(int playerId, int slotIndex) {
    if (!gameStarted || playerId <= 0 || slotIndex < 0) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->assignPlayerToSlot(static_cast<PlayerId>(playerId), static_cast<std::size_t>(slotIndex))) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::swapEditableLineupSlots(int firstSlotIndex, int secondSlotIndex) {
    if (!gameStarted || firstSlotIndex < 0 || secondSlotIndex < 0) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->swapSlots(static_cast<std::size_t>(firstSlotIndex),
                           static_cast<std::size_t>(secondSlotIndex))) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::clearEditableLineupSlot(int slotIndex) {
    if (!gameStarted || slotIndex < 0) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->clearSlot(static_cast<std::size_t>(slotIndex))) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::unassignEditableLineupPlayer(int playerId) {
    if (!gameStarted || playerId <= 0) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->unassignPlayer(static_cast<PlayerId>(playerId))) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

int GameFacade::getEditableLineupSubstituteCapacity() const {
    return static_cast<int>(kMaxSubstituteCount);
}

bool GameFacade::assignEditableLineupPlayerToSubstitute(int playerId, int substituteIndex) {
    if (!gameStarted || playerId <= 0 || substituteIndex < 0) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->assignPlayerToSubstitute(static_cast<PlayerId>(playerId),
                                          static_cast<std::size_t>(substituteIndex))) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::clearEditableLineupSubstitute(int substituteIndex) {
    if (!gameStarted || substituteIndex < 0) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->clearSubstitute(static_cast<std::size_t>(substituteIndex))) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::swapEditableLineupSubstitutes(int firstSubstituteIndex, int secondSubstituteIndex) {
    if (!gameStarted || firstSubstituteIndex < 0 || secondSubstituteIndex < 0) {
        return false;
    }

    EditableLineup* lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup && !ensureEditableLineupReady()) {
        return false;
    }
    lineup = editableLineup.has_value() ? &editableLineup.value() : nullptr;
    if (!lineup) {
        return false;
    }

    if (!lineup->swapSubstitutes(static_cast<std::size_t>(firstSubstituteIndex),
                                 static_cast<std::size_t>(secondSubstituteIndex))) {
        return false;
    }

    if (!syncEditableLineupToSelectedTeamSheet(false)) {
        return false;
    }
    refreshEditableLineupViews();
    emit gameStateChanged();
    return true;
}

bool GameFacade::applyEditableLineupToActivePreMatch() {
    if (!gameStarted) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame || !currentGame->getActivePreMatchInteraction()) {
        return false;
    }
    if (!ensureSelectedTeamContext()) {
        setLastError(QStringLiteral("No managed team is available for lineup application."));
        publishGameStateChanged();
        return false;
    }
    if (!ensureEditableLineupReady() || !editableLineup.has_value()) {
        setLastError(QStringLiteral("Lineup is not ready. Current pre-match lineup unchanged."));
        publishGameStateChanged();
        return false;
    }

    const TeamId managedTeamId = currentGame->getManagedTeamId();
    if (managedTeamId == 0 || editableLineup->getTeamId() != managedTeamId) {
        setLastError(QStringLiteral("Lineup team does not match the managed team."));
        publishGameStateChanged();
        return false;
    }

    const std::optional<TeamSheet> teamSheet = editableLineup->toTeamSheetIfComplete();
    if (!teamSheet.has_value()) {
        setLastError(QStringLiteral("Lineup incomplete. Current pre-match lineup unchanged."));
        publishGameStateChanged();
        return false;
    }

    if (!currentGame->replacePendingPreMatchTeamSheetForTeam(managedTeamId, *teamSheet)) {
        setLastError(QStringLiteral("Could not apply lineup to the active pre-match."));
        publishGameStateChanged();
        return false;
    }

    currentGame->flushSaveState();
    setLastError(QString());
    publishGameStateChanged();
    return true;
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

QVariantMap GameFacade::getMatchReportDetails(qulonglong matchId) const {
    if (!gameStarted || matchId == 0 || !hasValidLeagueSelection()) {
        return {};
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return {};
    }

    const MatchReport* report = league->findCurrentSeasonMatchReportById(static_cast<MatchId>(matchId));
    if (!report) {
        return {};
    }

    return toMatchReportDetailsMap(*report, league);
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
        row.matchId = static_cast<qulonglong>(record.matchId);
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

    const Team* selectedTeam = league->findTeamById(selectedTeamId);
    const QString selectedTeamName = fromStd(league->getTeamName(selectedTeamId));
    const QString selectedPrimaryColor = primaryColorForTeam(selectedTeam);
    dashboardStateObject.setRootValues(
        true,
        selectedTeamName,
        selectedPrimaryColor,
        secondaryColorForTeam(selectedTeam),
        badgeTextColorForTeam(selectedTeam),
        formatDate(ensureGame()->getDate()),
        formatGameState(ensureGame()->getState()),
        managerName,
        fromStd(league->getRecentFormString(selectedTeamId, 5))
    );

    if (const std::optional<FixtureMatchPreview> nextMatch = league->getNextMatchForTeam(selectedTeamId);
        nextMatch.has_value()) {
        const Team* homeTeam = league->findTeamById(nextMatch->homeId);
        const Team* awayTeam = league->findTeamById(nextMatch->awayId);
        const QString homePrimaryColor = primaryColorForTeam(homeTeam);
        const QString awayPrimaryColor = primaryColorForTeam(awayTeam);
        dashboardStateObject.nextMatch()->setFromValues(
            formatDate(nextMatch->date),
            fromStd(league->getTeamName(nextMatch->homeId)),
            homePrimaryColor,
            secondaryColorForTeam(homeTeam),
            badgeTextColorForTeam(homeTeam),
            fromStd(league->getTeamName(nextMatch->awayId)),
            awayPrimaryColor,
            secondaryColorForTeam(awayTeam),
            badgeTextColorForTeam(awayTeam),
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
        const Team* homeTeam = league->findTeamById(preview.homeId);
        const Team* awayTeam = league->findTeamById(preview.awayId);
        const QString homePrimaryColor = primaryColorForTeam(homeTeam);
        const QString awayPrimaryColor = primaryColorForTeam(awayTeam);
        row.dateText = formatDate(preview.date);
        row.homeTeamName = fromStd(league->getTeamName(preview.homeId));
        row.homePrimaryColor = homePrimaryColor;
        row.homeSecondaryColor = secondaryColorForTeam(homeTeam);
        row.homeTextColor = badgeTextColorForTeam(homeTeam);
        row.awayTeamName = fromStd(league->getTeamName(preview.awayId));
        row.awayPrimaryColor = awayPrimaryColor;
        row.awaySecondaryColor = secondaryColorForTeam(awayTeam);
        row.awayTextColor = badgeTextColorForTeam(awayTeam);
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

    const League* league = resolveLeague(selectedLeagueId);
    const Team* selectedTeam = league ? league->findTeamById(selectedTeamId) : nullptr;
    const QString selectedPrimaryColor = primaryColorForTeam(selectedTeam);
    shellStateObject.setFromValues(
        true,
        getSelectedTeamName(),
        selectedPrimaryColor,
        secondaryColorForTeam(selectedTeam),
        badgeTextColorForTeam(selectedTeam),
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
        TeamPresentationBuilder teamBuilder;
        TeamSheetPresentationBuilder sheetBuilder(teamBuilder);
        MatchPresentationBuilder matchBuilder(sheetBuilder);
        const PreMatchViewDto view = matchBuilder.buildPreMatch(*preMatch, league);
        const QString formationText =
            view.homeTeamId == selectedTeamId ? fromStd(view.home.formationText)
            : (view.awayTeamId == selectedTeamId ? fromStd(view.away.formationText) : QString());
        interactionStateObject.preMatch()->setFromValues(
            static_cast<qulonglong>(view.matchId),
            fromStd(view.dateText),
            view.matchweek,
            static_cast<int>(view.homeTeamId),
            static_cast<int>(view.awayTeamId),
            fromStd(view.home.team.name),
            fromStd(view.away.team.name),
            fromStd(view.home.team.primaryColor),
            fromStd(view.home.team.secondaryColor),
            fromStd(view.home.team.textColor),
            fromStd(view.away.team.primaryColor),
            fromStd(view.away.team.secondaryColor),
            fromStd(view.away.team.textColor),
            fromStd(view.home.coachName),
            fromStd(view.away.coachName),
            fromStd(view.homeRecentForm),
            fromStd(view.awayRecentForm),
            fromStd(view.home.formationText),
            fromStd(view.away.formationText),
            formationText,
            fromStd(view.home.summary.averageOverallText),
            fromStd(view.away.summary.averageOverallText),
            toLineupVariantList(view.home, selectedTeamId),
            toLineupVariantList(view.away, selectedTeamId)
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
        QString scorerSummary = QStringLiteral("No goals recorded.");
        QString assistSummary = QStringLiteral("No assists recorded.");
        QString cardSummary = QStringLiteral("No cards recorded.");
        QVariantList statRows;
        TeamPresentationBuilder teamBuilder;
        TeamSheetPresentationBuilder sheetBuilder(teamBuilder);
        MatchPresentationBuilder matchBuilder(sheetBuilder);
        const PostMatchViewDto view = matchBuilder.buildPostMatch(*postMatch, league);
        if (league) {
            if (const MatchReport* report = league->findCurrentSeasonMatchReportById(postMatch->getMatchId())) {
                const Team* homeTeam = league->findTeamById(report->homeId);
                const Team* awayTeam = league->findTeamById(report->awayId);
                scorerSummary = formatScorerSummary(*report, homeTeam, awayTeam);
                assistSummary = formatAssistSummary(*report, homeTeam, awayTeam);
                cardSummary = formatCardSummary(*report, homeTeam, awayTeam);
                statRows = buildMatchStatRows(*report);
            }
        }
        interactionStateObject.postMatch()->setFromValues(
            static_cast<qulonglong>(view.matchId),
            fromStd(view.dateText),
            view.matchweek,
            static_cast<int>(view.homeTeamId),
            static_cast<int>(view.awayTeamId),
            fromStd(view.home.team.name),
            fromStd(view.away.team.name),
            fromStd(view.home.team.primaryColor),
            fromStd(view.home.team.secondaryColor),
            fromStd(view.home.team.textColor),
            fromStd(view.away.team.primaryColor),
            fromStd(view.away.team.secondaryColor),
            fromStd(view.away.team.textColor),
            view.homeGoals,
            view.awayGoals,
            scorerSummary,
            assistSummary,
            cardSummary,
            fromStd(view.home.coachName),
            fromStd(view.away.coachName),
            fromStd(view.home.formationText),
            fromStd(view.away.formationText),
            fromStd(view.home.summary.averageOverallText),
            fromStd(view.away.summary.averageOverallText),
            toLineupVariantList(view.home, selectedTeamId),
            toLineupVariantList(view.away, selectedTeamId),
            statRows
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

void GameFacade::refreshEditableLineup() {
    if (!ensureSelectedTeamContext()) {
        const Game* currentGame = game.get();
        qWarning() << "[GameFacade::refreshEditableLineup] Invalid selected team context."
                   << "gameStarted=" << gameStarted
                   << "selectedLeagueId=" << selectedLeagueId
                   << "selectedTeamId=" << selectedTeamId
                   << "managedLeagueId=" << (currentGame ? currentGame->getManagedLeagueId() : 0)
                   << "managedTeamId=" << (currentGame ? currentGame->getManagedTeamId() : 0);
        clearEditableLineupData();
        return;
    }

    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        qWarning() << "[GameFacade::refreshEditableLineup] League not found for id:" << selectedLeagueId;
        clearEditableLineupData();
        return;
    }

    const Team* team = league->findTeamById(selectedTeamId);
    if (!team) {
        qWarning() << "[GameFacade::refreshEditableLineup] Team not found in league."
                   << "teamId=" << selectedTeamId
                   << "leagueId=" << selectedLeagueId;
        clearEditableLineupData();
        return;
    }

    const Game* currentGame = game.get();
    const std::optional<TeamSheet> selectedTeamSheet =
        currentGame ? currentGame->getSelectedTeamSheetForTeam(selectedLeagueId, selectedTeamId) : std::nullopt;
    const FormationId preferredFormation = selectedTeamSheet.has_value()
        ? selectedTeamSheet->formation
        : FormationId::FourThreeThree;
    const bool hasLineupForSelectedTeam = editableLineup.has_value()
        && editableLineup->getTeamId() == selectedTeamId;
    const FormationId activeFormation = hasLineupForSelectedTeam
        ? editableLineup->getFormationId()
        : preferredFormation;
    const int expectedSlotCount = isFormationSupported(activeFormation)
        ? static_cast<int>(getFormationSlotTemplate(activeFormation).size())
        : 0;
    const bool needsBuild = !hasLineupForSelectedTeam
        || expectedSlotCount == 0
        || static_cast<int>(editableLineup->getSlots().size()) != expectedSlotCount;
    if (needsBuild) {
        editableLineup = EditableLineup(*team, preferredFormation);
    }
    if (editableLineup.has_value() && selectedTeamSheet.has_value()) {
        if (editableLineup->getFormationId() != selectedTeamSheet->formation) {
            editableLineup = EditableLineup(*team, selectedTeamSheet->formation);
        }
        editableLineup->applyTeamSheet(*selectedTeamSheet);
    }

    refreshEditableLineupViews();

    if (editableLineupSlotsModel.rowCount() != expectedSlotCount) {
        qWarning() << "[GameFacade::refreshEditableLineup] Slots model count mismatch after refresh."
                   << "teamId=" << selectedTeamId
                   << "slots=" << editableLineupSlotsModel.rowCount()
                   << "expectedSlots=" << expectedSlotCount;
    }
    const int selectedTeamPlayerCount = getSelectedTeamPlayerCount();
    if (editableLineupRosterModel.rowCount() != selectedTeamPlayerCount) {
        qWarning() << "[GameFacade::refreshEditableLineup] Roster model count mismatch after refresh."
                   << "teamId=" << selectedTeamId
                   << "roster=" << editableLineupRosterModel.rowCount()
                   << "expectedRoster=" << selectedTeamPlayerCount;
    }
}

void GameFacade::clearEditableLineupData() {
    editableLineup.reset();
    editableLineupStateObject.clear();
    editableLineupSlotsModel.clear();
    editableLineupRosterModel.clear();
    editableLineupSubstitutesModel.clear();
}

void GameFacade::refreshEditableLineupViews() {
    refreshEditableLineupSlotsModel();
    refreshEditableLineupRosterModel();
    refreshEditableLineupSubstitutesModel();
    refreshEditableLineupStateObject();
}

void GameFacade::refreshEditableLineupStateObject() {
    const EditableLineup* lineup = resolveEditableLineup();
    if (!lineup) {
        editableLineupStateObject.clear();
        return;
    }

    int assignedCount = 0;
    for (const EditableLineupSlot& slot : lineup->getSlots()) {
        if (slot.assignedPlayerId != 0) {
            ++assignedCount;
        }
    }

    editableLineupStateObject.setFromValues(
        true,
        static_cast<int>(lineup->getTeamId()),
        static_cast<int>(lineup->getFormationId()),
        formatFormation(lineup->getFormationId()),
        static_cast<int>(lineup->getSlots().size()),
        editableLineupRosterModel.rowCount(),
        assignedCount,
        lineup->isFullLineup());
}

void GameFacade::refreshEditableLineupSlotsModel() {
    const League* league = resolveLeague(selectedLeagueId);
    const EditableLineup* lineup = resolveEditableLineup();
    const Team* team = (league && lineup) ? league->findTeamById(lineup->getTeamId()) : nullptr;
    if (!lineup || !team) {
        editableLineupSlotsModel.clear();
        return;
    }

    QVector<EditableLineupSlotsModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(lineup->getSlots().size()));
    for (const EditableLineupSlot& slot : lineup->getSlots()) {
        EditableLineupSlotsModel::Row row;
        row.slotIndex = static_cast<int>(slot.slotIndex);
        row.slotRole = formatSlotRole(slot.slotRole);
        row.slotRoleKey = static_cast<int>(slot.slotRole);
        row.slotRoleText = formatSlotRole(slot.slotRole);
        row.slotLabel = formatSlotRole(slot.slotRole);

        const Footballer* assignedPlayer = slot.assignedPlayerId != 0
            ? team->findPlayerById(slot.assignedPlayerId)
            : nullptr;

        row.isEmpty = assignedPlayer == nullptr;
        row.hasAssignedPlayer = assignedPlayer != nullptr;
        row.assignedPlayerId = assignedPlayer ? static_cast<int>(assignedPlayer->getId()) : 0;
        row.assignedPlayerName = assignedPlayer ? fromStd(assignedPlayer->getName()) : QString();
        row.assignedPlayerOverall = assignedPlayer ? assignedPlayer->totalPower() : 0;
        row.assignedPlayerOverallSummary =
            assignedPlayer ? QStringLiteral("OVR %1").arg(assignedPlayer->totalPower()) : QString();

        const PlayerConditionState* condition = assignedPlayer ? &assignedPlayer->getConditionState() : nullptr;
        row.assignedPlayerForm = condition ? condition->getForm() : 0;
        row.assignedPlayerFitness = condition ? condition->getFitness() : 0;
        row.assignedPlayerMorale = condition ? condition->getMorale() : 0;

        const std::optional<FormationPitchCoordinate> pitchCoordinate = getFormationPitchCoordinate(
            lineup->getFormationId(),
            slot.slotIndex,
            slot.slotRole);
        const FormationPitchCoordinate coordinate = pitchCoordinate.value_or(
            getFallbackFormationPitchCoordinate(slot.slotIndex, lineup->getSlots().size(), slot.slotRole));
        if (!pitchCoordinate.has_value() && !warnedUnsupportedEditableLineupPitchLayout) {
            warnedUnsupportedEditableLineupPitchLayout = true;
            qWarning() << "[GameFacade::refreshEditableLineupSlotsModel] Falling back to generic pitch coordinates."
                       << "formation=" << static_cast<int>(lineup->getFormationId())
                       << "slotIndex=" << static_cast<int>(slot.slotIndex)
                       << "slotRole=" << static_cast<int>(slot.slotRole)
                       << "slotCount=" << static_cast<int>(lineup->getSlots().size());
        }
        row.pitchX = coordinate.x;
        row.pitchY = coordinate.y;
        row.displayOrder = coordinate.displayOrder;
        rows.push_back(row);
    }

    editableLineupSlotsModel.setRows(std::move(rows));
}

void GameFacade::refreshEditableLineupRosterModel() {
    const League* league = resolveLeague(selectedLeagueId);
    const EditableLineup* lineup = resolveEditableLineup();
    const Team* team = (league && lineup) ? league->findTeamById(lineup->getTeamId()) : nullptr;
    if (!lineup || !team) {
        editableLineupRosterModel.clear();
        return;
    }

    std::vector<const Footballer*> rosterPlayers;
    rosterPlayers.reserve(team->getPlayers().size());
    for (const auto& player : team->getPlayers()) {
        if (!player) {
            continue;
        }
        rosterPlayers.push_back(player.get());
    }

    std::sort(rosterPlayers.begin(), rosterPlayers.end(), [](const Footballer* lhs, const Footballer* rhs) {
        return lhs->getId() < rhs->getId();
    });

    QVector<EditableLineupRosterModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(rosterPlayers.size()));
    for (const Footballer* playerPtr : rosterPlayers) {
        if (!playerPtr) {
            continue;
        }

        const Footballer& player = *playerPtr;
        const std::optional<std::size_t> assignedSlot = lineup->findAssignedSlotIndex(player.getId());
        const PlayerConditionState& condition = player.getConditionState();

        EditableLineupRosterModel::Row row;
        row.playerId = static_cast<int>(player.getId());
        row.name = fromStd(player.getName());
        row.positionShort = formatPositionShortCode(player);
        row.overall = player.totalPower();
        row.overallSummary = QStringLiteral("OVR %1").arg(player.totalPower());
        row.form = condition.getForm();
        row.fitness = condition.getFitness();
        row.morale = condition.getMorale();
        row.isAssigned = assignedSlot.has_value();
        row.assignedSlotIndex = assignedSlot.has_value() ? static_cast<int>(*assignedSlot) : -1;
        rows.push_back(row);
    }

    editableLineupRosterModel.setRows(std::move(rows));
}

void GameFacade::refreshEditableLineupSubstitutesModel() {
    const League* league = resolveLeague(selectedLeagueId);
    const EditableLineup* lineup = resolveEditableLineup();
    const Team* team = (league && lineup) ? league->findTeamById(lineup->getTeamId()) : nullptr;
    if (!lineup || !team) {
        editableLineupSubstitutesModel.clear();
        return;
    }

    QVector<EditableLineupRosterModel::Row> rows;
    rows.reserve(static_cast<qsizetype>(kMaxSubstituteCount));
    for (std::size_t substituteIndex = 0; substituteIndex < kMaxSubstituteCount; ++substituteIndex) {
        const PlayerId playerId = substituteIndex < lineup->getSubstitutePlayerIds().size()
            ? lineup->getSubstitutePlayerIds()[substituteIndex]
            : 0;
        if (playerId == 0) {
            EditableLineupRosterModel::Row row;
            row.positionShort = QStringLiteral("SUB");
            row.isAssigned = true;
            row.assignedSlotIndex = -1;
            row.substituteIndex = static_cast<int>(substituteIndex);
            row.hasPlayer = false;
            rows.push_back(row);
            continue;
        }

        const Footballer* playerPtr = team->findPlayerById(playerId);
        if (!playerPtr) {
            EditableLineupRosterModel::Row row;
            row.positionShort = QStringLiteral("SUB");
            row.isAssigned = true;
            row.assignedSlotIndex = -1;
            row.substituteIndex = static_cast<int>(substituteIndex);
            row.hasPlayer = false;
            rows.push_back(row);
            continue;
        }

        const Footballer& player = *playerPtr;
        const PlayerConditionState& condition = player.getConditionState();

        EditableLineupRosterModel::Row row;
        row.playerId = static_cast<int>(player.getId());
        row.name = fromStd(player.getName());
        row.positionShort = formatPositionShortCode(player);
        row.overall = player.totalPower();
        row.overallSummary = QStringLiteral("OVR %1").arg(player.totalPower());
        row.form = condition.getForm();
        row.fitness = condition.getFitness();
        row.morale = condition.getMorale();
        row.isAssigned = true;
        row.assignedSlotIndex = -1;
        row.substituteIndex = static_cast<int>(substituteIndex);
        row.hasPlayer = true;
        rows.push_back(row);
    }

    editableLineupSubstitutesModel.setRows(std::move(rows));
}

bool GameFacade::syncEditableLineupToSelectedTeamSheet(bool requireComplete) {
    if (!gameStarted || !editableLineup.has_value() || selectedLeagueId == 0 || selectedTeamId == 0) {
        return false;
    }

    Game* currentGame = ensureGame();
    if (!currentGame) {
        return false;
    }

    TeamSheet teamSheet;
    if (requireComplete) {
        const std::optional<TeamSheet> completeTeamSheet = editableLineup->toTeamSheetIfComplete();
        if (!completeTeamSheet.has_value()) {
            return false;
        }
        teamSheet = *completeTeamSheet;
    } else {
        teamSheet = editableLineup->exportAsTeamSheet();
    }

    if (!currentGame->updateSelectedTeamSheetForTeam(selectedLeagueId, selectedTeamId, teamSheet)) {
        return false;
    }

    currentGame->flushSaveState();
    return true;
}

const EditableLineup* GameFacade::resolveEditableLineup() const {
    GameFacade* self = const_cast<GameFacade*>(this);
    if (!self->editableLineup.has_value()) {
        self->refreshEditableLineup();
    }

    return self->editableLineup.has_value() ? &self->editableLineup.value() : nullptr;
}

int GameFacade::getSelectedTeamPlayerCount() const {
    const League* league = resolveLeague(selectedLeagueId);
    if (!league) {
        return 0;
    }

    const Team* team = league->findTeamById(selectedTeamId);
    if (!team) {
        return 0;
    }

    int count = 0;
    for (const auto& player : team->getPlayers()) {
        if (player) {
            ++count;
        }
    }
    return count;
}

int GameFacade::getExpectedEditableLineupSlotCount() const {
    const EditableLineup* lineup = resolveEditableLineup();
    if (lineup) {
        return static_cast<int>(lineup->getSlots().size());
    }

    const League* league = resolveLeague(selectedLeagueId);
    const Team* team = league ? league->findTeamById(selectedTeamId) : nullptr;
    if (!team) {
        return 0;
    }

    return static_cast<int>(getFormationSlotTemplate(team->getHeadCoach().getPreferredFormation()).size());
}

void GameFacade::publishGameStateChanged() {
    ensureSelectedTeamContext();
    ensureEditableLineupReady();
    syncEditableLineupDisplayToActivePreMatch();
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

void GameFacade::syncEditableLineupDisplayToActivePreMatch() {
    if (!gameStarted || !editableLineup.has_value()) {
        return;
    }

    Game* currentGame = ensureGame();
    if (!currentGame || !currentGame->getActivePreMatchInteraction()) {
        return;
    }

    const TeamId managedTeamId = currentGame->getManagedTeamId();
    if (managedTeamId == 0 || editableLineup->getTeamId() != managedTeamId) {
        return;
    }

    TeamSheet displaySheet;
    displaySheet.teamId = managedTeamId;
    displaySheet.coachId = editableLineup->getCoachId();
    displaySheet.formation = editableLineup->getFormationId();
    displaySheet.tacticalSetup = editableLineup->getTacticalSetup();
    displaySheet.startingAssignments.reserve(editableLineup->getSlots().size());
    displaySheet.startingPlayerIds.reserve(editableLineup->getSlots().size());

    for (const EditableLineupSlot& slot : editableLineup->getSlots()) {
        displaySheet.startingAssignments.push_back(TeamSheetSlotAssignment{ slot.slotIndex, slot.slotRole, slot.assignedPlayerId });
        if (slot.assignedPlayerId != 0) {
            displaySheet.startingPlayerIds.push_back(slot.assignedPlayerId);
        }
    }

    for (PlayerId playerId : editableLineup->getSubstitutePlayerIds()) {
        if (playerId != 0) {
            displaySheet.substitutePlayerIds.push_back(playerId);
        }
    }
    displaySheet.teamId = managedTeamId;
    currentGame->replaceActivePreMatchDisplayTeamSheetForTeam(managedTeamId, displaySheet);
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


QString GameFacade::formatFormation(FormationId formation) const {
    switch (formation) {
    case FormationId::FourFourTwo:
        return QStringLiteral("4-4-2");
    case FormationId::FourThreeThree:
        return QStringLiteral("4-3-3");
    case FormationId::ThreeFiveTwo:
        return QStringLiteral("3-5-2");
    }

    return QStringLiteral("Unknown");
}

QString GameFacade::formatSlotRole(FormationSlotRole role) const {
    switch (role) {
    case FormationSlotRole::Goalkeeper:
        return QStringLiteral("GK");
    case FormationSlotRole::LeftBack:
        return QStringLiteral("LB");
    case FormationSlotRole::CenterBack:
        return QStringLiteral("CB");
    case FormationSlotRole::RightBack:
        return QStringLiteral("RB");
    case FormationSlotRole::LeftWingBack:
        return QStringLiteral("LWB");
    case FormationSlotRole::RightWingBack:
        return QStringLiteral("RWB");
    case FormationSlotRole::DefensiveMidfielder:
        return QStringLiteral("DM");
    case FormationSlotRole::CentralMidfielder:
        return QStringLiteral("CM");
    case FormationSlotRole::AttackingMidfielder:
        return QStringLiteral("AM");
    case FormationSlotRole::LeftMidfielder:
        return QStringLiteral("LM");
    case FormationSlotRole::RightMidfielder:
        return QStringLiteral("RM");
    case FormationSlotRole::LeftWinger:
        return QStringLiteral("LW");
    case FormationSlotRole::RightWinger:
        return QStringLiteral("RW");
    case FormationSlotRole::Striker:
        return QStringLiteral("ST");
    case FormationSlotRole::Unknown:
        break;
    }

    return QStringLiteral("?");
}

QString GameFacade::formatPositionShortCode(const Footballer& player) const {
    switch (player.getPlayerPosition()) {
    case PlayerPosition::Goalkeeper:
        return QStringLiteral("GK");
    case PlayerPosition::CenterBack:
        return QStringLiteral("CB");
    case PlayerPosition::LeftBack:
        return QStringLiteral("LB");
    case PlayerPosition::RightBack:
        return QStringLiteral("RB");
    case PlayerPosition::DefensiveMidfielder:
        return QStringLiteral("DM");
    case PlayerPosition::CentralMidfielder:
        return QStringLiteral("CM");
    case PlayerPosition::AttackingMidfielder:
        return QStringLiteral("AM");
    case PlayerPosition::LeftMidfielder:
        return QStringLiteral("LM");
    case PlayerPosition::RightMidfielder:
        return QStringLiteral("RM");
    case PlayerPosition::LeftWinger:
        return QStringLiteral("LW");
    case PlayerPosition::RightWinger:
        return QStringLiteral("RW");
    case PlayerPosition::Striker:
        return QStringLiteral("ST");
    case PlayerPosition::Unknown:
        break;
    }

    return QStringLiteral("?");
}

QVariantMap GameFacade::toNextMatchMap(const FixtureMatchPreview& preview) const {
    QVariantMap map;
    const League* league = resolveLeague(selectedLeagueId); 
    const Team* homeTeam = league ? league->findTeamById(preview.homeId) : nullptr;
    const Team* awayTeam = league ? league->findTeamById(preview.awayId) : nullptr;
    map.insert(QStringLiteral("hasNextMatch"), true);
    map.insert(QStringLiteral("dateText"), formatDate(preview.date));
    map.insert(QStringLiteral("homeTeamId"), static_cast<int>(preview.homeId));
    map.insert(QStringLiteral("awayTeamId"), static_cast<int>(preview.awayId));
    map.insert(QStringLiteral("homeTeamName"), league ? fromStd(league->getTeamName(preview.homeId)) : QString());
    map.insert(QStringLiteral("awayTeamName"), league ? fromStd(league->getTeamName(preview.awayId)) : QString());
    insertTeamVisualFields(map, QStringLiteral("home"), homeTeam);
    insertTeamVisualFields(map, QStringLiteral("away"), awayTeam);
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
    map.insert(QStringLiteral("matchId"), static_cast<qulonglong>(record.matchId));
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

QVariantMap GameFacade::toMatchReportDetailsMap(const MatchReport& report, const League* league) const {
    QVariantMap map;
    const Team* homeTeam = league ? league->findTeamById(report.homeId) : nullptr;
    const Team* awayTeam = league ? league->findTeamById(report.awayId) : nullptr;
    TeamPresentationBuilder teamBuilder;
    TeamSheetPresentationBuilder sheetBuilder(teamBuilder);
    const TeamSheetViewDto homeSheet = sheetBuilder.buildFromMatchLineupSnapshot(report.homeLineup, homeTeam);
    const TeamSheetViewDto awaySheet = sheetBuilder.buildFromMatchLineupSnapshot(report.awayLineup, awayTeam);
    map.insert(QStringLiteral("matchId"), static_cast<qulonglong>(report.matchId));
    map.insert(QStringLiteral("dateText"), formatDate(report.date));
    insertTeamSheetFields(map, QStringLiteral("home"), homeSheet, selectedTeamId);
    insertTeamSheetFields(map, QStringLiteral("away"), awaySheet, selectedTeamId);
    map.insert(QStringLiteral("score"), QStringLiteral("%1 - %2").arg(report.homeGoals).arg(report.awayGoals));
    map.insert(QStringLiteral("matchweek"), report.matchweek);
    map.insert(QStringLiteral("scorers"), formatScorerSummary(report, homeTeam, awayTeam));
    map.insert(QStringLiteral("assists"), formatAssistSummary(report, homeTeam, awayTeam));
    map.insert(QStringLiteral("cards"), formatCardSummary(report, homeTeam, awayTeam));
    map.insert(QStringLiteral("statRows"), buildMatchStatRows(report));

    std::vector<MatchEventRecord> orderedEvents = report.events;
    std::stable_sort(
        orderedEvents.begin(),
        orderedEvents.end(),
        [](const MatchEventRecord& lhs, const MatchEventRecord& rhs) {
            return lhs.minute < rhs.minute;
        });

    QVariantList eventList;
    eventList.reserve(static_cast<qsizetype>(orderedEvents.size()));
    for (const MatchEventRecord& event : orderedEvents) {
        QVariantMap eventMap;
        eventMap.insert(QStringLiteral("minute"), event.minute);
        eventMap.insert(QStringLiteral("kind"), eventKindText(event.kind));
        eventMap.insert(QStringLiteral("teamId"), static_cast<int>(event.teamId));

        const QString teamName = league ? fromStd(league->getTeamName(event.teamId)) : QString();
        eventMap.insert(QStringLiteral("teamName"), teamName);

        const QString primaryName = resolveMatchPlayerName(
            homeTeam,
            awayTeam,
            report.homeId,
            report.awayId,
            event.teamId,
            event.primaryPlayerId);
        const QString secondaryName = resolveMatchPlayerName(
            homeTeam,
            awayTeam,
            report.homeId,
            report.awayId,
            event.teamId,
            event.secondaryPlayerId);
        eventMap.insert(QStringLiteral("primaryPlayerName"), primaryName);
        eventMap.insert(QStringLiteral("secondaryPlayerName"), secondaryName);

        QString detailText = QStringLiteral("%1' %2").arg(event.minute).arg(eventKindText(event.kind));
        if (!teamName.isEmpty()) {
            detailText += QStringLiteral(" - %1").arg(teamName);
        }
        if (!primaryName.isEmpty()) {
            detailText += QStringLiteral(" - %1").arg(primaryName);
        }
        if (!secondaryName.isEmpty()) {
            detailText += QStringLiteral(" (Assist: %1)").arg(secondaryName);
        }
        eventMap.insert(QStringLiteral("detailText"), detailText);
        eventList.push_back(eventMap);
    }

    map.insert(QStringLiteral("events"), eventList);
    map.insert(QStringLiteral("hasData"), true);
    return map;
}

QVariantMap GameFacade::toPreMatchInteractionMap(const PreMatchInteraction& interaction) const {
    const League* league = resolveLeague(interaction.getLeagueId());
    TeamPresentationBuilder teamBuilder;
    TeamSheetPresentationBuilder sheetBuilder(teamBuilder);
    MatchPresentationBuilder matchBuilder(sheetBuilder);
    const PreMatchViewDto view = matchBuilder.buildPreMatch(interaction, league);

    QVariantMap map;
    map.insert(QStringLiteral("kind"), QStringLiteral("pre_match"));
    map.insert(QStringLiteral("hasData"), view.hasData);
    map.insert(QStringLiteral("matchId"), static_cast<qulonglong>(view.matchId));
    map.insert(QStringLiteral("dateText"), fromStd(view.dateText));
    map.insert(QStringLiteral("matchweek"), view.matchweek);
    map.insert(QStringLiteral("homeTeamId"), static_cast<int>(view.homeTeamId));
    map.insert(QStringLiteral("awayTeamId"), static_cast<int>(view.awayTeamId));
    map.insert(QStringLiteral("home"), toVariantMap(view.home, selectedTeamId));
    map.insert(QStringLiteral("away"), toVariantMap(view.away, selectedTeamId));
    insertTeamSheetFields(map, QStringLiteral("home"), view.home, selectedTeamId);
    insertTeamSheetFields(map, QStringLiteral("away"), view.away, selectedTeamId);
    map.insert(QStringLiteral("homeRecentForm"), fromStd(view.homeRecentForm));
    map.insert(QStringLiteral("awayRecentForm"), fromStd(view.awayRecentForm));
    if (view.homeTeamId == selectedTeamId) {
        map.insert(QStringLiteral("formationText"), fromStd(view.home.formationText));
    } else if (view.awayTeamId == selectedTeamId) {
        map.insert(QStringLiteral("formationText"), fromStd(view.away.formationText));
    }

    return map;
}

QVariantMap GameFacade::toPostMatchInteractionMap(const PostMatchInteraction& interaction) const {
    const League* league = resolveLeague(interaction.getLeagueId());
    TeamPresentationBuilder teamBuilder;
    TeamSheetPresentationBuilder sheetBuilder(teamBuilder);
    MatchPresentationBuilder matchBuilder(sheetBuilder);
    const PostMatchViewDto view = matchBuilder.buildPostMatch(interaction, league);

    QVariantMap map;
    map.insert(QStringLiteral("kind"), QStringLiteral("post_match"));
    map.insert(QStringLiteral("hasData"), view.hasData);
    map.insert(QStringLiteral("matchId"), static_cast<qulonglong>(view.matchId));
    map.insert(QStringLiteral("dateText"), fromStd(view.dateText));
    map.insert(QStringLiteral("matchweek"), view.matchweek);
    map.insert(QStringLiteral("homeTeamId"), static_cast<int>(view.homeTeamId));
    map.insert(QStringLiteral("awayTeamId"), static_cast<int>(view.awayTeamId));
    map.insert(QStringLiteral("homeGoals"), view.homeGoals);
    map.insert(QStringLiteral("awayGoals"), view.awayGoals);
    map.insert(QStringLiteral("home"), toVariantMap(view.home, selectedTeamId));
    map.insert(QStringLiteral("away"), toVariantMap(view.away, selectedTeamId));
    insertTeamSheetFields(map, QStringLiteral("home"), view.home, selectedTeamId);
    insertTeamSheetFields(map, QStringLiteral("away"), view.away, selectedTeamId);
    if (league) {
        if (const MatchReport* report = league->findCurrentSeasonMatchReportById(interaction.getMatchId())) {
            const Team* homeTeam = league->findTeamById(report->homeId);
            const Team* awayTeam = league->findTeamById(report->awayId);
            map.insert(QStringLiteral("scorerSummary"), formatScorerSummary(*report, homeTeam, awayTeam));
            map.insert(QStringLiteral("assistSummary"), formatAssistSummary(*report, homeTeam, awayTeam));
            map.insert(QStringLiteral("cardSummary"), formatCardSummary(*report, homeTeam, awayTeam));
            map.insert(QStringLiteral("statRows"), buildMatchStatRows(*report));
        }
    }
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
