#include "fm/data/SqliteGameStateRepository.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
    constexpr int GameStateRowId = 1;

    std::string currentUtcTimestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm utcTime{};
#if defined(_WIN32)
        gmtime_s(&utcTime, &nowTime);
#else
        gmtime_r(&utcTime, &nowTime);
#endif

        std::ostringstream output;
        output << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
        return output.str();
    }

    std::string dateToIsoString(const Date& date) {
        std::ostringstream output;
        output << std::setw(4) << std::setfill('0') << date.getYear() << "-"
            << std::setw(2) << std::setfill('0') << static_cast<int>(date.getMonth()) << "-"
            << std::setw(2) << std::setfill('0') << date.getDay();
        return output.str();
    }

    Date parseDate(const std::string& value) {
        if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
            throw std::runtime_error("invalid persisted game date: " + value);
        }
        const int year = std::stoi(value.substr(0, 4));
        const int month = std::stoi(value.substr(5, 2));
        const int day = std::stoi(value.substr(8, 2));
        if (month < 1 || month > 12) {
            throw std::runtime_error("invalid persisted game date month: " + value);
        }
        const Month parsedMonth = static_cast<Month>(month);
        if (day < 1 || day > Date::daysInMonth(year, parsedMonth)) {
            throw std::runtime_error("invalid persisted game date day: " + value);
        }
        return Date(year, parsedMonth, day);
    }

    std::string joinIds(const std::vector<PlayerId>& values) {
        std::ostringstream output;
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                output << ",";
            }
            output << values[i];
        }
        return output.str();
    }

    std::vector<PlayerId> splitIds(const std::string& value) {
        std::vector<PlayerId> ids;
        std::istringstream input(value);
        std::string token;
        while (std::getline(input, token, ',')) {
            if (!token.empty()) {
                ids.push_back(static_cast<PlayerId>(std::stoi(token)));
            }
        }
        return ids;
    }

    bool tableExists(const SqliteDatabase& database, const std::string& tableName) {
        SqliteStatement statement = database.prepare(
            "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = ?");
        statement.bindText(1, tableName);
        if (!statement.stepRow()) {
            return false;
        }
        return statement.columnInt(0) > 0;
    }

    void ensureSchema(const SqliteDatabase& database) {
        database.execute(
            "CREATE TABLE IF NOT EXISTS game_state ("
            "id INTEGER PRIMARY KEY CHECK (id = 1),"
            "\"current_date\" TEXT NOT NULL,"
            "current_state INTEGER NOT NULL DEFAULT 0,"
            "updated_at_utc TEXT NOT NULL"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS league_runtime_state ("
            "league_id INTEGER PRIMARY KEY,"
            "season_year INTEGER NOT NULL,"
            "is_fixture_generated INTEGER NOT NULL,"
            "last_season_rollover_year INTEGER NOT NULL DEFAULT -1"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_fixtures ("
            "match_id INTEGER PRIMARY KEY,"
            "league_id INTEGER NOT NULL,"
            "season_year INTEGER NOT NULL,"
            "matchweek INTEGER NOT NULL,"
            "match_date TEXT NOT NULL,"
            "home_team_id INTEGER NOT NULL,"
            "away_team_id INTEGER NOT NULL,"
            "played INTEGER NOT NULL,"
            "event_enqueued INTEGER NOT NULL DEFAULT 0,"
            "home_goals INTEGER NOT NULL DEFAULT -1,"
            "away_goals INTEGER NOT NULL DEFAULT -1"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_match_reports ("
            "match_id INTEGER PRIMARY KEY,"
            "league_id INTEGER NOT NULL,"
            "season_year INTEGER NOT NULL,"
            "match_date TEXT NOT NULL,"
            "home_team_id INTEGER NOT NULL,"
            "away_team_id INTEGER NOT NULL,"
            "matchweek INTEGER NOT NULL,"
            "home_goals INTEGER NOT NULL,"
            "away_goals INTEGER NOT NULL,"
            "home_coach_id INTEGER NOT NULL,"
            "home_formation INTEGER NOT NULL,"
            "home_starting_player_ids TEXT NOT NULL,"
            "away_coach_id INTEGER NOT NULL,"
            "away_formation INTEGER NOT NULL,"
            "away_starting_player_ids TEXT NOT NULL"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_match_player_reports ("
            "match_id INTEGER NOT NULL,"
            "player_id INTEGER NOT NULL,"
            "team_id INTEGER NOT NULL,"
            "started INTEGER NOT NULL,"
            "minutes_played INTEGER NOT NULL,"
            "goals INTEGER NOT NULL,"
            "assists INTEGER NOT NULL,"
            "yellow_cards INTEGER NOT NULL,"
            "red_cards INTEGER NOT NULL,"
            "PRIMARY KEY (match_id, player_id)"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_match_events ("
            "match_id INTEGER NOT NULL,"
            "event_index INTEGER NOT NULL,"
            "minute INTEGER NOT NULL,"
            "kind INTEGER NOT NULL,"
            "team_id INTEGER NOT NULL,"
            "primary_player_id INTEGER NOT NULL,"
            "secondary_player_id INTEGER NOT NULL,"
            "PRIMARY KEY (match_id, event_index)"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS player_runtime_state ("
            "player_id INTEGER PRIMARY KEY,"
            "form INTEGER NOT NULL,"
            "fitness INTEGER NOT NULL,"
            "morale INTEGER NOT NULL"
            ");");
        database.execute("PRAGMA user_version = 2;");
    }

    MatchReport readReportRow(SqliteStatement& statement) {
        MatchReport report;
        report.matchId = static_cast<MatchId>(statement.columnInt64(0));
        report.leagueId = static_cast<LeagueId>(statement.columnInt(1));
        report.seasonYear = statement.columnInt(2);
        report.date = parseDate(statement.columnText(3));
        report.homeId = static_cast<TeamId>(statement.columnInt(4));
        report.awayId = static_cast<TeamId>(statement.columnInt(5));
        report.matchweek = statement.columnInt(6);
        report.homeGoals = statement.columnInt(7);
        report.awayGoals = statement.columnInt(8);
        report.homeLineup.teamId = report.homeId;
        report.homeLineup.coachId = static_cast<CoachId>(statement.columnInt(9));
        report.homeLineup.formation = static_cast<FormationId>(statement.columnInt(10));
        report.homeLineup.startingPlayerIds = splitIds(statement.columnText(11));
        report.awayLineup.teamId = report.awayId;
        report.awayLineup.coachId = static_cast<CoachId>(statement.columnInt(12));
        report.awayLineup.formation = static_cast<FormationId>(statement.columnInt(13));
        report.awayLineup.startingPlayerIds = splitIds(statement.columnText(14));
        return report;
    }
}

SqliteGameStateRepository::SqliteGameStateRepository(
    const std::string& databasePath,
    GameStateRepositoryMode mode)
    : database(databasePath) {
    if (mode == GameStateRepositoryMode::EnsureSchema) {
        ensureSchema(database);
    }
}

bool SqliteGameStateRepository::hasGameState() const {
    if (!tableExists(database, "game_state")) {
        return false;
    }

    SqliteStatement statement = database.prepare("SELECT \"current_date\" FROM game_state WHERE id = ?");
    statement.bindInt(1, GameStateRowId);
    if (!statement.stepRow()) {
        return false;
    }

    try {
        (void)parseDate(statement.columnText(0));
        return true;
    } catch (...) {
        return false;
    }
}

Date SqliteGameStateRepository::loadCurrentDate() const {
    SqliteStatement statement = database.prepare("SELECT \"current_date\" FROM game_state WHERE id = ?");
    statement.bindInt(1, GameStateRowId);
    if (!statement.stepRow()) {
        throw std::runtime_error("runtime game_state row does not exist");
    }
    return parseDate(statement.columnText(0));
}

std::vector<PersistedLeagueRuntimeState> SqliteGameStateRepository::loadLeagueRuntimeStates() const {
    std::vector<PersistedLeagueRuntimeState> states;
    SqliteStatement statement = database.prepare(
        "SELECT league_id, season_year, is_fixture_generated, last_season_rollover_year "
        "FROM league_runtime_state ORDER BY league_id");
    while (statement.stepRow()) {
        PersistedLeagueRuntimeState state;
        state.leagueId = static_cast<LeagueId>(statement.columnInt(0));
        state.seasonYear = statement.columnInt(1);
        state.fixtureGenerated = statement.columnInt(2) != 0;
        state.lastSeasonRolloverYear = statement.columnInt(3);
        states.push_back(state);
    }
    return states;
}

std::vector<PersistedFixtureState> SqliteGameStateRepository::loadFixtures() const {
    std::vector<PersistedFixtureState> fixtures;
    SqliteStatement statement = database.prepare(
        "SELECT match_id, league_id, season_year, matchweek, match_date, home_team_id, away_team_id, "
        "played, event_enqueued, home_goals, away_goals "
        "FROM runtime_fixtures ORDER BY league_id, match_date, match_id");
    while (statement.stepRow()) {
        PersistedFixtureState fixture;
        fixture.matchId = static_cast<MatchId>(statement.columnInt64(0));
        fixture.leagueId = static_cast<LeagueId>(statement.columnInt(1));
        fixture.seasonYear = statement.columnInt(2);
        fixture.matchweek = statement.columnInt(3);
        fixture.date = parseDate(statement.columnText(4));
        fixture.homeTeamId = static_cast<TeamId>(statement.columnInt(5));
        fixture.awayTeamId = static_cast<TeamId>(statement.columnInt(6));
        fixture.played = statement.columnInt(7) != 0;
        fixture.eventEnqueued = statement.columnInt(8) != 0;
        fixture.homeGoals = statement.columnInt(9);
        fixture.awayGoals = statement.columnInt(10);
        fixtures.push_back(fixture);
    }
    return fixtures;
}

std::vector<MatchReport> SqliteGameStateRepository::loadMatchReports() const {
    std::vector<MatchReport> reports;
    {
        SqliteStatement statement = database.prepare(
            "SELECT match_id, league_id, season_year, match_date, home_team_id, away_team_id, matchweek, "
            "home_goals, away_goals, home_coach_id, home_formation, home_starting_player_ids, "
            "away_coach_id, away_formation, away_starting_player_ids "
            "FROM runtime_match_reports ORDER BY league_id, match_date, match_id");
        while (statement.stepRow()) {
            reports.push_back(readReportRow(statement));
        }
    }

    for (MatchReport& report : reports) {
        SqliteStatement playerStatement = database.prepare(
            "SELECT player_id, team_id, started, minutes_played, goals, assists, yellow_cards, red_cards "
            "FROM runtime_match_player_reports WHERE match_id = ? ORDER BY player_id");
        playerStatement.bindInt64(1, static_cast<std::int64_t>(report.matchId));
        while (playerStatement.stepRow()) {
            MatchPlayerReport playerReport;
            playerReport.playerId = static_cast<PlayerId>(playerStatement.columnInt(0));
            playerReport.teamId = static_cast<TeamId>(playerStatement.columnInt(1));
            playerReport.started = playerStatement.columnInt(2) != 0;
            playerReport.minutesPlayed = playerStatement.columnInt(3);
            playerReport.goals = playerStatement.columnInt(4);
            playerReport.assists = playerStatement.columnInt(5);
            playerReport.yellowCards = playerStatement.columnInt(6);
            playerReport.redCards = playerStatement.columnInt(7);
            report.playerReports.push_back(playerReport);
        }

        SqliteStatement eventStatement = database.prepare(
            "SELECT minute, kind, team_id, primary_player_id, secondary_player_id "
            "FROM runtime_match_events WHERE match_id = ? ORDER BY event_index");
        eventStatement.bindInt64(1, static_cast<std::int64_t>(report.matchId));
        while (eventStatement.stepRow()) {
            MatchEventRecord event;
            event.minute = eventStatement.columnInt(0);
            event.kind = static_cast<MatchEventKind>(eventStatement.columnInt(1));
            event.teamId = static_cast<TeamId>(eventStatement.columnInt(2));
            event.primaryPlayerId = static_cast<PlayerId>(eventStatement.columnInt(3));
            event.secondaryPlayerId = static_cast<PlayerId>(eventStatement.columnInt(4));
            report.events.push_back(event);
        }
    }

    return reports;
}

std::vector<PersistedPlayerRuntimeState> SqliteGameStateRepository::loadPlayerRuntimeStates() const {
    std::vector<PersistedPlayerRuntimeState> playerStates;
    SqliteStatement statement = database.prepare(
        "SELECT player_id, form, fitness, morale FROM player_runtime_state ORDER BY player_id");
    while (statement.stepRow()) {
        PersistedPlayerRuntimeState state;
        state.playerId = static_cast<PlayerId>(statement.columnInt(0));
        state.form = statement.columnInt(1);
        state.fitness = statement.columnInt(2);
        state.morale = statement.columnInt(3);
        playerStates.push_back(state);
    }
    return playerStates;
}

void SqliteGameStateRepository::updateCurrentDate(const Date& currentDate) const {
    SqliteStatement statement = database.prepare(
        "UPDATE game_state SET \"current_date\" = ?, updated_at_utc = ? WHERE id = ?");
    statement.bindText(1, dateToIsoString(currentDate));
    statement.bindText(2, currentUtcTimestamp());
    statement.bindInt(3, GameStateRowId);
    statement.stepDone();
}

void SqliteGameStateRepository::saveRuntimeState(
    const Date& currentDate,
    int currentState,
    const std::vector<PersistedLeagueRuntimeState>& leagueStates,
    const std::vector<PersistedFixtureState>& fixtures,
    const std::vector<MatchReport>& reports,
    const std::vector<PersistedPlayerRuntimeState>& playerStates) const {
    database.execute("BEGIN TRANSACTION;");

    try {
        database.execute("DELETE FROM runtime_match_events;");
        database.execute("DELETE FROM runtime_match_player_reports;");
        database.execute("DELETE FROM runtime_match_reports;");
        database.execute("DELETE FROM runtime_fixtures;");
        database.execute("DELETE FROM league_runtime_state;");
        database.execute("DELETE FROM player_runtime_state;");
        database.execute("DELETE FROM game_state;");

        {
            SqliteStatement statement = database.prepare(
                "INSERT INTO game_state (id, \"current_date\", current_state, updated_at_utc) "
                "VALUES (?, ?, ?, ?)");
            statement.bindInt(1, GameStateRowId);
            statement.bindText(2, dateToIsoString(currentDate));
            statement.bindInt(3, currentState);
            statement.bindText(4, currentUtcTimestamp());
            statement.stepDone();
        }

        for (const PersistedLeagueRuntimeState& state : leagueStates) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO league_runtime_state ("
                "league_id, season_year, is_fixture_generated, last_season_rollover_year"
                ") VALUES (?, ?, ?, ?)");
            statement.bindInt(1, static_cast<int>(state.leagueId));
            statement.bindInt(2, state.seasonYear);
            statement.bindInt(3, state.fixtureGenerated ? 1 : 0);
            statement.bindInt(4, state.lastSeasonRolloverYear);
            statement.stepDone();
        }

        for (const PersistedFixtureState& fixture : fixtures) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_fixtures ("
                "match_id, league_id, season_year, matchweek, match_date, home_team_id, away_team_id, "
                "played, event_enqueued, home_goals, away_goals"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt64(1, static_cast<std::int64_t>(fixture.matchId));
            statement.bindInt(2, static_cast<int>(fixture.leagueId));
            statement.bindInt(3, fixture.seasonYear);
            statement.bindInt(4, fixture.matchweek);
            statement.bindText(5, dateToIsoString(fixture.date));
            statement.bindInt(6, static_cast<int>(fixture.homeTeamId));
            statement.bindInt(7, static_cast<int>(fixture.awayTeamId));
            statement.bindInt(8, fixture.played ? 1 : 0);
            statement.bindInt(9, fixture.eventEnqueued ? 1 : 0);
            statement.bindInt(10, fixture.homeGoals);
            statement.bindInt(11, fixture.awayGoals);
            statement.stepDone();
        }

        for (const MatchReport& report : reports) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_match_reports ("
                "match_id, league_id, season_year, match_date, home_team_id, away_team_id, matchweek, "
                "home_goals, away_goals, home_coach_id, home_formation, home_starting_player_ids, "
                "away_coach_id, away_formation, away_starting_player_ids"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt64(1, static_cast<std::int64_t>(report.matchId));
            statement.bindInt(2, static_cast<int>(report.leagueId));
            statement.bindInt(3, report.seasonYear);
            statement.bindText(4, dateToIsoString(report.date));
            statement.bindInt(5, static_cast<int>(report.homeId));
            statement.bindInt(6, static_cast<int>(report.awayId));
            statement.bindInt(7, report.matchweek);
            statement.bindInt(8, report.homeGoals);
            statement.bindInt(9, report.awayGoals);
            statement.bindInt(10, static_cast<int>(report.homeLineup.coachId));
            statement.bindInt(11, static_cast<int>(report.homeLineup.formation));
            statement.bindText(12, joinIds(report.homeLineup.startingPlayerIds));
            statement.bindInt(13, static_cast<int>(report.awayLineup.coachId));
            statement.bindInt(14, static_cast<int>(report.awayLineup.formation));
            statement.bindText(15, joinIds(report.awayLineup.startingPlayerIds));
            statement.stepDone();

            for (const MatchPlayerReport& playerReport : report.playerReports) {
                SqliteStatement playerStatement = database.prepare(
                    "INSERT INTO runtime_match_player_reports ("
                    "match_id, player_id, team_id, started, minutes_played, goals, assists, yellow_cards, red_cards"
                    ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
                playerStatement.bindInt64(1, static_cast<std::int64_t>(report.matchId));
                playerStatement.bindInt(2, static_cast<int>(playerReport.playerId));
                playerStatement.bindInt(3, static_cast<int>(playerReport.teamId));
                playerStatement.bindInt(4, playerReport.started ? 1 : 0);
                playerStatement.bindInt(5, playerReport.minutesPlayed);
                playerStatement.bindInt(6, playerReport.goals);
                playerStatement.bindInt(7, playerReport.assists);
                playerStatement.bindInt(8, playerReport.yellowCards);
                playerStatement.bindInt(9, playerReport.redCards);
                playerStatement.stepDone();
            }

            for (std::size_t i = 0; i < report.events.size(); ++i) {
                const MatchEventRecord& event = report.events[i];
                SqliteStatement eventStatement = database.prepare(
                    "INSERT INTO runtime_match_events ("
                    "match_id, event_index, minute, kind, team_id, primary_player_id, secondary_player_id"
                    ") VALUES (?, ?, ?, ?, ?, ?, ?)");
                eventStatement.bindInt64(1, static_cast<std::int64_t>(report.matchId));
                eventStatement.bindInt(2, static_cast<int>(i));
                eventStatement.bindInt(3, event.minute);
                eventStatement.bindInt(4, static_cast<int>(event.kind));
                eventStatement.bindInt(5, static_cast<int>(event.teamId));
                eventStatement.bindInt(6, static_cast<int>(event.primaryPlayerId));
                eventStatement.bindInt(7, static_cast<int>(event.secondaryPlayerId));
                eventStatement.stepDone();
            }
        }

        for (const PersistedPlayerRuntimeState& state : playerStates) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO player_runtime_state (player_id, form, fitness, morale) VALUES (?, ?, ?, ?)");
            statement.bindInt(1, static_cast<int>(state.playerId));
            statement.bindInt(2, state.form);
            statement.bindInt(3, state.fitness);
            statement.bindInt(4, state.morale);
            statement.stepDone();
        }

        database.execute("COMMIT;");
    } catch (...) {
        try {
            database.execute("ROLLBACK;");
        } catch (...) {
        }
        throw;
    }
}
