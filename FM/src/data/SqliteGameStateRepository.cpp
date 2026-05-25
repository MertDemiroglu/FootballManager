#include "fm/data/SqliteGameStateRepository.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
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

    std::string toStableCode(FormationId formation) {
        switch (formation) {
        case FormationId::FourFourTwo:
            return "4-4-2";
        case FormationId::FourThreeThree:
            return "4-3-3";
        case FormationId::ThreeFiveTwo:
            return "3-5-2";
        }

        throw std::runtime_error("cannot persist unsupported formation");
    }

    std::optional<FormationId> formationFromStableCode(const std::string& stableCode) {
        if (stableCode == "4-4-2") {
            return FormationId::FourFourTwo;
        }
        if (stableCode == "4-3-3") {
            return FormationId::FourThreeThree;
        }
        if (stableCode == "3-5-2") {
            return FormationId::ThreeFiveTwo;
        }
        return std::nullopt;
    }

    std::string toStableCode(FormationSlotRole role) {
        switch (role) {
        case FormationSlotRole::Goalkeeper:
            return "gk";
        case FormationSlotRole::CenterBack:
            return "cb";
        case FormationSlotRole::LeftBack:
            return "lb";
        case FormationSlotRole::RightBack:
            return "rb";
        case FormationSlotRole::LeftWingBack:
            return "lwb";
        case FormationSlotRole::RightWingBack:
            return "rwb";
        case FormationSlotRole::DefensiveMidfielder:
            return "dm";
        case FormationSlotRole::CentralMidfielder:
            return "cm";
        case FormationSlotRole::AttackingMidfielder:
            return "am";
        case FormationSlotRole::LeftMidfielder:
            return "lm";
        case FormationSlotRole::RightMidfielder:
            return "rm";
        case FormationSlotRole::LeftWinger:
            return "lw";
        case FormationSlotRole::RightWinger:
            return "rw";
        case FormationSlotRole::Striker:
            return "st";
        case FormationSlotRole::Unknown:
            break;
        }

        throw std::runtime_error("cannot persist unsupported slot role");
    }

    std::optional<FormationSlotRole> slotRoleFromStableCode(const std::string& stableCode) {
        if (stableCode == "gk") return FormationSlotRole::Goalkeeper;
        if (stableCode == "cb") return FormationSlotRole::CenterBack;
        if (stableCode == "lb") return FormationSlotRole::LeftBack;
        if (stableCode == "rb") return FormationSlotRole::RightBack;
        if (stableCode == "lwb") return FormationSlotRole::LeftWingBack;
        if (stableCode == "rwb") return FormationSlotRole::RightWingBack;
        if (stableCode == "dm") return FormationSlotRole::DefensiveMidfielder;
        if (stableCode == "cm") return FormationSlotRole::CentralMidfielder;
        if (stableCode == "am") return FormationSlotRole::AttackingMidfielder;
        if (stableCode == "lm") return FormationSlotRole::LeftMidfielder;
        if (stableCode == "rm") return FormationSlotRole::RightMidfielder;
        if (stableCode == "lw") return FormationSlotRole::LeftWinger;
        if (stableCode == "rw") return FormationSlotRole::RightWinger;
        if (stableCode == "st") return FormationSlotRole::Striker;
        return std::nullopt;
    }

    TeamMentality parseMentalityOrThrow(const std::string& stableCode) {
        std::optional<TeamMentality> mentality = teamMentalityFromStableCode(stableCode);
        if (!mentality.has_value()) {
            throw std::runtime_error("runtime team sheet has invalid mentality stable code: " + stableCode);
        }
        return *mentality;
    }

    TeamTempo parseTempoOrThrow(const std::string& stableCode) {
        std::optional<TeamTempo> tempo = teamTempoFromStableCode(stableCode);
        if (!tempo.has_value()) {
            throw std::runtime_error("runtime team sheet has invalid tempo stable code: " + stableCode);
        }
        return *tempo;
    }

    TeamWidth parseWidthOrThrow(const std::string& stableCode) {
        std::optional<TeamWidth> width = teamWidthFromStableCode(stableCode);
        if (!width.has_value()) {
            throw std::runtime_error("runtime team sheet has invalid width stable code: " + stableCode);
        }
        return *width;
    }

    DefensiveLine parseDefensiveLineOrThrow(const std::string& stableCode) {
        std::optional<DefensiveLine> defensiveLine = defensiveLineFromStableCode(stableCode);
        if (!defensiveLine.has_value()) {
            throw std::runtime_error("runtime team sheet has invalid defensive_line stable code: " + stableCode);
        }
        return *defensiveLine;
    }

    PressingIntensity parsePressingIntensityOrThrow(const std::string& stableCode) {
        std::optional<PressingIntensity> pressingIntensity = pressingIntensityFromStableCode(stableCode);
        if (!pressingIntensity.has_value()) {
            throw std::runtime_error("runtime team sheet has invalid pressing_intensity stable code: " + stableCode);
        }
        return *pressingIntensity;
    }

    MarkingStyle parseMarkingStyleOrThrow(const std::string& stableCode) {
        std::optional<MarkingStyle> markingStyle = markingStyleFromStableCode(stableCode);
        if (!markingStyle.has_value()) {
            throw std::runtime_error("runtime team sheet has invalid marking_style stable code: " + stableCode);
        }
        return *markingStyle;
    }

    PassingDirectness parsePassingDirectnessOrThrow(const std::string& stableCode) {
        std::optional<PassingDirectness> passingDirectness = passingDirectnessFromStableCode(stableCode);
        if (!passingDirectness.has_value()) {
            throw std::runtime_error("runtime team sheet has invalid passing_directness stable code: " + stableCode);
        }
        return *passingDirectness;
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

    bool columnExists(const SqliteDatabase& database, const std::string& tableName, const std::string& columnName) {
        SqliteStatement statement = database.prepare("PRAGMA table_info(" + tableName + ")");
        while (statement.stepRow()) {
            if (statement.columnText(1) == columnName) {
                return true;
            }
        }
        return false;
    }

    void addColumnIfMissing(
        const SqliteDatabase& database,
        const std::string& tableName,
        const std::string& columnDefinition) {
        const std::string columnName = columnDefinition.substr(0, columnDefinition.find(' '));
        if (tableExists(database, tableName) && !columnExists(database, tableName, columnName)) {
            database.execute("ALTER TABLE " + tableName + " ADD COLUMN " + columnDefinition + ";");
        }
    }

    void requireRuntimeTeamSheetsTacticalSetupV1Schema(const SqliteDatabase& database) {
        const std::vector<std::string> requiredColumns{
            "width",
            "defensive_line",
            "pressing_intensity",
            "marking_style",
            "passing_directness"
        };

        for (const std::string& columnName : requiredColumns) {
            if (!columnExists(database, "runtime_team_sheets", columnName)) {
                throw std::runtime_error(
                    "runtime_team_sheets is missing " + columnName
                    + "; clear incompatible development saves or start a new game");
            }
        }
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
            "CREATE TABLE IF NOT EXISTS runtime_save_settings ("
            "id INTEGER PRIMARY KEY CHECK (id = 1),"
            "auto_save_frequency TEXT NOT NULL,"
            "last_auto_save_date TEXT"
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
            "away_starting_player_ids TEXT NOT NULL,"
            "home_shots INTEGER NOT NULL DEFAULT 0,"
            "home_shots_on_target INTEGER NOT NULL DEFAULT 0,"
            "home_passes_attempted INTEGER NOT NULL DEFAULT 0,"
            "home_passes_completed INTEGER NOT NULL DEFAULT 0,"
            "home_tackles_attempted INTEGER NOT NULL DEFAULT 0,"
            "home_tackles_won INTEGER NOT NULL DEFAULT 0,"
            "home_interceptions INTEGER NOT NULL DEFAULT 0,"
            "home_fouls INTEGER NOT NULL DEFAULT 0,"
            "home_corners INTEGER NOT NULL DEFAULT 0,"
            "home_yellow_cards INTEGER NOT NULL DEFAULT 0,"
            "home_red_cards INTEGER NOT NULL DEFAULT 0,"
            "home_possession_share REAL NOT NULL DEFAULT 0.0,"
            "home_expected_goals REAL NOT NULL DEFAULT 0.0,"
            "away_shots INTEGER NOT NULL DEFAULT 0,"
            "away_shots_on_target INTEGER NOT NULL DEFAULT 0,"
            "away_passes_attempted INTEGER NOT NULL DEFAULT 0,"
            "away_passes_completed INTEGER NOT NULL DEFAULT 0,"
            "away_tackles_attempted INTEGER NOT NULL DEFAULT 0,"
            "away_tackles_won INTEGER NOT NULL DEFAULT 0,"
            "away_interceptions INTEGER NOT NULL DEFAULT 0,"
            "away_fouls INTEGER NOT NULL DEFAULT 0,"
            "away_corners INTEGER NOT NULL DEFAULT 0,"
            "away_yellow_cards INTEGER NOT NULL DEFAULT 0,"
            "away_red_cards INTEGER NOT NULL DEFAULT 0,"
            "away_possession_share REAL NOT NULL DEFAULT 0.0,"
            "away_expected_goals REAL NOT NULL DEFAULT 0.0"
            ");");
        addColumnIfMissing(database, "runtime_match_reports", "home_shots INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_shots_on_target INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_passes_attempted INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_passes_completed INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_tackles_attempted INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_tackles_won INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_interceptions INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_fouls INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_corners INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_yellow_cards INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_red_cards INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "home_possession_share REAL NOT NULL DEFAULT 0.0");
        addColumnIfMissing(database, "runtime_match_reports", "home_expected_goals REAL NOT NULL DEFAULT 0.0");
        addColumnIfMissing(database, "runtime_match_reports", "away_shots INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_shots_on_target INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_passes_attempted INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_passes_completed INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_tackles_attempted INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_tackles_won INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_interceptions INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_fouls INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_corners INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_yellow_cards INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_red_cards INTEGER NOT NULL DEFAULT 0");
        addColumnIfMissing(database, "runtime_match_reports", "away_possession_share REAL NOT NULL DEFAULT 0.0");
        addColumnIfMissing(database, "runtime_match_reports", "away_expected_goals REAL NOT NULL DEFAULT 0.0");
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
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_player_attributes ("
            "player_id INTEGER PRIMARY KEY,"
            "shooting INTEGER NOT NULL,"
            "passing INTEGER NOT NULL,"
            "first_touch INTEGER NOT NULL,"
            "technique INTEGER NOT NULL,"
            "tackling INTEGER NOT NULL,"
            "dribbling INTEGER NOT NULL,"
            "crossing INTEGER NOT NULL,"
            "marking INTEGER NOT NULL,"
            "heading INTEGER NOT NULL,"
            "set_pieces INTEGER NOT NULL,"
            "decisions INTEGER NOT NULL,"
            "vision INTEGER NOT NULL,"
            "positioning INTEGER NOT NULL,"
            "off_the_ball INTEGER NOT NULL,"
            "composure INTEGER NOT NULL,"
            "concentration INTEGER NOT NULL,"
            "work_rate INTEGER NOT NULL,"
            "teamwork INTEGER NOT NULL,"
            "leadership INTEGER NOT NULL,"
            "aggression INTEGER NOT NULL,"
            "pace INTEGER NOT NULL,"
            "acceleration INTEGER NOT NULL,"
            "stamina INTEGER NOT NULL,"
            "strength INTEGER NOT NULL,"
            "agility INTEGER NOT NULL,"
            "jumping_reach INTEGER NOT NULL,"
            "shot_stopping INTEGER NOT NULL,"
            "handling INTEGER NOT NULL,"
            "aerial_ability INTEGER NOT NULL,"
            "one_on_ones INTEGER NOT NULL,"
            "distribution INTEGER NOT NULL"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_team_sheets ("
            "league_id INTEGER NOT NULL,"
            "team_id INTEGER NOT NULL,"
            "formation TEXT NOT NULL,"
            "mentality TEXT NOT NULL,"
            "tempo TEXT NOT NULL,"
            "width TEXT NOT NULL DEFAULT 'balanced',"
            "defensive_line TEXT NOT NULL DEFAULT 'standard',"
            "pressing_intensity TEXT NOT NULL DEFAULT 'normal',"
            "marking_style TEXT NOT NULL DEFAULT 'mixed',"
            "passing_directness TEXT NOT NULL DEFAULT 'balanced',"
            "PRIMARY KEY (league_id, team_id)"
            ");");
        requireRuntimeTeamSheetsTacticalSetupV1Schema(database);
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_team_sheet_starters ("
            "league_id INTEGER NOT NULL,"
            "team_id INTEGER NOT NULL,"
            "slot_index INTEGER NOT NULL,"
            "slot_role TEXT NOT NULL,"
            "player_id INTEGER NOT NULL,"
            "PRIMARY KEY (league_id, team_id, slot_index)"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_team_sheet_substitutes ("
            "league_id INTEGER NOT NULL,"
            "team_id INTEGER NOT NULL,"
            "substitute_order INTEGER NOT NULL,"
            "player_id INTEGER NOT NULL,"
            "PRIMARY KEY (league_id, team_id, substitute_order)"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_transfer_offers ("
            "offer_id INTEGER PRIMARY KEY,"
            "created_at TEXT NOT NULL,"
            "last_valid_date TEXT NOT NULL,"
            "expiry_policy TEXT NOT NULL,"
            "seller_league_id INTEGER NOT NULL,"
            "seller_team_id INTEGER NOT NULL,"
            "buyer_league_id INTEGER NOT NULL,"
            "buyer_team_id INTEGER NOT NULL,"
            "player_id INTEGER NOT NULL,"
            "fee INTEGER NOT NULL,"
            "status TEXT NOT NULL,"
            "resolution TEXT,"
            "FOREIGN KEY (seller_league_id) REFERENCES leagues(id),"
            "FOREIGN KEY (buyer_league_id) REFERENCES leagues(id),"
            "FOREIGN KEY (seller_team_id) REFERENCES teams(id),"
            "FOREIGN KEY (buyer_team_id) REFERENCES teams(id),"
            "FOREIGN KEY (player_id) REFERENCES players(id)"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_team_finances ("
            "league_id INTEGER NOT NULL,"
            "team_id INTEGER NOT NULL,"
            "total_budget INTEGER NOT NULL,"
            "transfer_budget INTEGER NOT NULL,"
            "wage_budget INTEGER NOT NULL,"
            "financial_strategy TEXT NOT NULL DEFAULT 'balanced',"
            "financial_health TEXT NOT NULL DEFAULT 'stable',"
            "PRIMARY KEY (league_id, team_id),"
            "FOREIGN KEY (league_id) REFERENCES leagues(id),"
            "FOREIGN KEY (team_id) REFERENCES teams(id)"
            ");");
        if (!columnExists(database, "runtime_team_finances", "financial_strategy")) {
            throw std::runtime_error(
                "runtime_team_finances is missing financial_strategy; clear incompatible development saves or start a new game");
        }
        if (!columnExists(database, "runtime_team_finances", "financial_health")) {
            throw std::runtime_error(
                "runtime_team_finances is missing financial_health; clear incompatible development saves or start a new game");
        }
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_player_roster_state ("
            "player_id INTEGER PRIMARY KEY,"
            "league_id INTEGER NOT NULL,"
            "team_id INTEGER NOT NULL,"
            "wage INTEGER,"
            "contract_years INTEGER,"
            "current_season_year INTEGER,"
            "FOREIGN KEY (league_id) REFERENCES leagues(id),"
            "FOREIGN KEY (team_id) REFERENCES teams(id),"
            "FOREIGN KEY (player_id) REFERENCES players(id)"
            ");");
        database.execute(
            "CREATE TABLE IF NOT EXISTS runtime_free_agents ("
            "player_id INTEGER PRIMARY KEY,"
            "previous_league_id INTEGER,"
            "previous_team_id INTEGER,"
            "became_free_agent_date TEXT,"
            "wage INTEGER,"
            "contract_years INTEGER,"
            "current_season_year INTEGER,"
            "FOREIGN KEY (player_id) REFERENCES players(id),"
            "FOREIGN KEY (previous_league_id) REFERENCES leagues(id),"
            "FOREIGN KEY (previous_team_id) REFERENCES teams(id)"
            ");");
        database.execute("PRAGMA user_version = 11;");
    }

    PersistedTransferOfferState readTransferOfferRow(SqliteStatement& statement) {
        PersistedTransferOfferState state;
        state.offerId = static_cast<OfferId>(statement.columnInt64(0));
        state.createdAt = parseDate(statement.columnText(1));
        state.lastValidDate = parseDate(statement.columnText(2));

        const std::string expiryPolicyCode = statement.columnText(3);
        const std::optional<TransferOfferExpiryPolicy> expiryPolicy = transferOfferExpiryPolicyFromStableCode(expiryPolicyCode);
        if (!expiryPolicy.has_value()) {
            throw std::runtime_error("runtime transfer offer has invalid expiry policy stable code: " + expiryPolicyCode);
        }
        state.expiryPolicy = *expiryPolicy;

        state.sellerLeagueId = static_cast<LeagueId>(statement.columnInt(4));
        state.sellerTeamId = static_cast<TeamId>(statement.columnInt(5));
        state.buyerLeagueId = static_cast<LeagueId>(statement.columnInt(6));
        state.buyerTeamId = static_cast<TeamId>(statement.columnInt(7));
        state.playerId = static_cast<PlayerId>(statement.columnInt(8));
        state.fee = statement.columnInt(9);

        const std::string statusCode = statement.columnText(10);
        const std::optional<TransferOfferStatus> status = transferOfferStatusFromStableCode(statusCode);
        if (!status.has_value()) {
            throw std::runtime_error("runtime transfer offer has invalid status stable code: " + statusCode);
        }
        state.status = *status;

        if (!statement.columnIsNull(11)) {
            const std::string resolutionCode = statement.columnText(11);
            const std::optional<TransferOfferResolution> resolution = transferOfferResolutionFromStableCode(resolutionCode);
            if (!resolution.has_value()) {
                throw std::runtime_error("runtime transfer offer has invalid resolution stable code: " + resolutionCode);
            }
            state.resolution = *resolution;
        }

        return state;
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
        report.homeStats.goals = report.homeGoals;
        report.awayStats.goals = report.awayGoals;
        return report;
    }

    MatchReport readReportRowWithStats(SqliteStatement& statement) {
        MatchReport report = readReportRow(statement);
        int column = 15;
        report.homeStats.shots = statement.columnInt(column++);
        report.homeStats.shotsOnTarget = statement.columnInt(column++);
        report.homeStats.passesAttempted = statement.columnInt(column++);
        report.homeStats.passesCompleted = statement.columnInt(column++);
        report.homeStats.tacklesAttempted = statement.columnInt(column++);
        report.homeStats.tacklesWon = statement.columnInt(column++);
        report.homeStats.interceptions = statement.columnInt(column++);
        report.homeStats.fouls = statement.columnInt(column++);
        report.homeStats.corners = statement.columnInt(column++);
        report.homeStats.yellowCards = statement.columnInt(column++);
        report.homeStats.redCards = statement.columnInt(column++);
        report.homeStats.possessionShare = statement.columnDouble(column++);
        report.homeStats.expectedGoals = statement.columnDouble(column++);
        report.awayStats.shots = statement.columnInt(column++);
        report.awayStats.shotsOnTarget = statement.columnInt(column++);
        report.awayStats.passesAttempted = statement.columnInt(column++);
        report.awayStats.passesCompleted = statement.columnInt(column++);
        report.awayStats.tacklesAttempted = statement.columnInt(column++);
        report.awayStats.tacklesWon = statement.columnInt(column++);
        report.awayStats.interceptions = statement.columnInt(column++);
        report.awayStats.fouls = statement.columnInt(column++);
        report.awayStats.corners = statement.columnInt(column++);
        report.awayStats.yellowCards = statement.columnInt(column++);
        report.awayStats.redCards = statement.columnInt(column++);
        report.awayStats.possessionShare = statement.columnDouble(column++);
        report.awayStats.expectedGoals = statement.columnDouble(column++);
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
        const bool hasTeamStats =
            columnExists(database, "runtime_match_reports", "home_shots")
            && columnExists(database, "runtime_match_reports", "away_expected_goals");
        if (hasTeamStats) {
            SqliteStatement statement = database.prepare(
                "SELECT match_id, league_id, season_year, match_date, home_team_id, away_team_id, matchweek, "
                "home_goals, away_goals, home_coach_id, home_formation, home_starting_player_ids, "
                "away_coach_id, away_formation, away_starting_player_ids, "
                "home_shots, home_shots_on_target, home_passes_attempted, home_passes_completed, "
                "home_tackles_attempted, home_tackles_won, home_interceptions, home_fouls, home_corners, "
                "home_yellow_cards, home_red_cards, home_possession_share, home_expected_goals, "
                "away_shots, away_shots_on_target, away_passes_attempted, away_passes_completed, "
                "away_tackles_attempted, away_tackles_won, away_interceptions, away_fouls, away_corners, "
                "away_yellow_cards, away_red_cards, away_possession_share, away_expected_goals "
                "FROM runtime_match_reports ORDER BY league_id, match_date, match_id");
            while (statement.stepRow()) {
                reports.push_back(readReportRowWithStats(statement));
            }
        } else {
            SqliteStatement statement = database.prepare(
                "SELECT match_id, league_id, season_year, match_date, home_team_id, away_team_id, matchweek, "
                "home_goals, away_goals, home_coach_id, home_formation, home_starting_player_ids, "
                "away_coach_id, away_formation, away_starting_player_ids "
                "FROM runtime_match_reports ORDER BY league_id, match_date, match_id");
            while (statement.stepRow()) {
                reports.push_back(readReportRow(statement));
            }
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

std::vector<PersistedTeamSheetState> SqliteGameStateRepository::loadTeamSheetStates() const {
    std::vector<PersistedTeamSheetState> states;
    if (!tableExists(database, "runtime_team_sheets")) {
        return states;
    }
    requireRuntimeTeamSheetsTacticalSetupV1Schema(database);

    {
        SqliteStatement statement = database.prepare(
            "SELECT league_id, team_id, formation, mentality, tempo, width, defensive_line, "
            "pressing_intensity, marking_style, passing_directness "
            "FROM runtime_team_sheets ORDER BY league_id, team_id");
        while (statement.stepRow()) {
            const LeagueId leagueId = static_cast<LeagueId>(statement.columnInt(0));
            const TeamId teamId = static_cast<TeamId>(statement.columnInt(1));
            const std::string formationCode = statement.columnText(2);
            const std::optional<FormationId> formation = formationFromStableCode(formationCode);
            if (!formation.has_value()) {
                throw std::runtime_error("runtime team sheet has invalid formation stable code: " + formationCode);
            }

            PersistedTeamSheetState state;
            state.leagueId = leagueId;
            state.teamSheet.teamId = teamId;
            state.teamSheet.formation = *formation;
            state.teamSheet.tacticalSetup.mentality = parseMentalityOrThrow(statement.columnText(3));
            state.teamSheet.tacticalSetup.tempo = parseTempoOrThrow(statement.columnText(4));
            state.teamSheet.tacticalSetup.width = parseWidthOrThrow(statement.columnText(5));
            state.teamSheet.tacticalSetup.defensiveLine = parseDefensiveLineOrThrow(statement.columnText(6));
            state.teamSheet.tacticalSetup.pressingIntensity = parsePressingIntensityOrThrow(statement.columnText(7));
            state.teamSheet.tacticalSetup.markingStyle = parseMarkingStyleOrThrow(statement.columnText(8));
            state.teamSheet.tacticalSetup.passingDirectness = parsePassingDirectnessOrThrow(statement.columnText(9));
            states.push_back(std::move(state));
        }
    }

    const auto findState = [&states](LeagueId leagueId, TeamId teamId) -> PersistedTeamSheetState* {
        for (PersistedTeamSheetState& state : states) {
            if (state.leagueId == leagueId && state.teamSheet.teamId == teamId) {
                return &state;
            }
        }
        return nullptr;
    };

    if (tableExists(database, "runtime_team_sheet_starters")) {
        SqliteStatement statement = database.prepare(
            "SELECT league_id, team_id, slot_index, slot_role, player_id "
            "FROM runtime_team_sheet_starters ORDER BY league_id, team_id, slot_index");
        while (statement.stepRow()) {
            const LeagueId leagueId = static_cast<LeagueId>(statement.columnInt(0));
            const TeamId teamId = static_cast<TeamId>(statement.columnInt(1));
            PersistedTeamSheetState* state = findState(leagueId, teamId);
            if (!state) {
                throw std::runtime_error("runtime team sheet starter references missing team sheet header");
            }

            const int slotIndex = statement.columnInt(2);
            const std::string slotRoleCode = statement.columnText(3);
            const std::optional<FormationSlotRole> slotRole = slotRoleFromStableCode(slotRoleCode);
            if (!slotRole.has_value()) {
                throw std::runtime_error("runtime team sheet starter has invalid slot role stable code: " + slotRoleCode);
            }

            const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(state->teamSheet.formation);
            if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= slotTemplate.size()) {
                throw std::runtime_error("runtime team sheet starter has invalid slot index");
            }
            if (slotTemplate[static_cast<std::size_t>(slotIndex)] != *slotRole) {
                throw std::runtime_error("runtime team sheet starter slot role does not match formation template");
            }

            const PlayerId playerId = static_cast<PlayerId>(statement.columnInt(4));
            state->teamSheet.startingAssignments.push_back(TeamSheetSlotAssignment{
                static_cast<std::size_t>(slotIndex),
                *slotRole,
                playerId
            });
            state->teamSheet.startingPlayerIds.push_back(playerId);
        }
    }

    if (tableExists(database, "runtime_team_sheet_substitutes")) {
        SqliteStatement statement = database.prepare(
            "SELECT league_id, team_id, substitute_order, player_id "
            "FROM runtime_team_sheet_substitutes ORDER BY league_id, team_id, substitute_order");
        while (statement.stepRow()) {
            const LeagueId leagueId = static_cast<LeagueId>(statement.columnInt(0));
            const TeamId teamId = static_cast<TeamId>(statement.columnInt(1));
            PersistedTeamSheetState* state = findState(leagueId, teamId);
            if (!state) {
                throw std::runtime_error("runtime team sheet substitute references missing team sheet header");
            }
            const int substituteOrder = statement.columnInt(2);
            if (substituteOrder < 0 || substituteOrder >= static_cast<int>(kMaxSubstituteCount)) {
                throw std::runtime_error("runtime team sheet substitute has invalid substitute order");
            }

            state->teamSheet.substitutePlayerIds.push_back(static_cast<PlayerId>(statement.columnInt(3)));
        }
    }

    return states;
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

std::vector<PersistedPlayerAttributesState> SqliteGameStateRepository::loadPlayerAttributesStates() const {
    std::vector<PersistedPlayerAttributesState> playerAttributesStates;
    if (!tableExists(database, "runtime_player_attributes")) {
        return playerAttributesStates;
    }

    SqliteStatement statement = database.prepare(
        "SELECT player_id, "
        "shooting, passing, first_touch, technique, tackling, dribbling, crossing, marking, heading, set_pieces, "
        "decisions, vision, positioning, off_the_ball, composure, concentration, work_rate, teamwork, leadership, aggression, "
        "pace, acceleration, stamina, strength, agility, jumping_reach, "
        "shot_stopping, handling, aerial_ability, one_on_ones, distribution "
        "FROM runtime_player_attributes ORDER BY player_id");
    while (statement.stepRow()) {
        PersistedPlayerAttributesState state;
        state.playerId = static_cast<PlayerId>(statement.columnInt(0));
        state.attributes.technical.shooting = statement.columnInt(1);
        state.attributes.technical.passing = statement.columnInt(2);
        state.attributes.technical.firstTouch = statement.columnInt(3);
        state.attributes.technical.technique = statement.columnInt(4);
        state.attributes.technical.tackling = statement.columnInt(5);
        state.attributes.technical.dribbling = statement.columnInt(6);
        state.attributes.technical.crossing = statement.columnInt(7);
        state.attributes.technical.marking = statement.columnInt(8);
        state.attributes.technical.heading = statement.columnInt(9);
        state.attributes.technical.setPieces = statement.columnInt(10);
        state.attributes.mental.decisions = statement.columnInt(11);
        state.attributes.mental.vision = statement.columnInt(12);
        state.attributes.mental.positioning = statement.columnInt(13);
        state.attributes.mental.offTheBall = statement.columnInt(14);
        state.attributes.mental.composure = statement.columnInt(15);
        state.attributes.mental.concentration = statement.columnInt(16);
        state.attributes.mental.workRate = statement.columnInt(17);
        state.attributes.mental.teamwork = statement.columnInt(18);
        state.attributes.mental.leadership = statement.columnInt(19);
        state.attributes.mental.aggression = statement.columnInt(20);
        state.attributes.physical.pace = statement.columnInt(21);
        state.attributes.physical.acceleration = statement.columnInt(22);
        state.attributes.physical.stamina = statement.columnInt(23);
        state.attributes.physical.strength = statement.columnInt(24);
        state.attributes.physical.agility = statement.columnInt(25);
        state.attributes.physical.jumpingReach = statement.columnInt(26);
        state.attributes.goalkeeper.shotStopping = statement.columnInt(27);
        state.attributes.goalkeeper.handling = statement.columnInt(28);
        state.attributes.goalkeeper.aerialAbility = statement.columnInt(29);
        state.attributes.goalkeeper.oneOnOnes = statement.columnInt(30);
        state.attributes.goalkeeper.distribution = statement.columnInt(31);
        validatePlayerAttributes(state.attributes);
        playerAttributesStates.push_back(state);
    }
    return playerAttributesStates;
}

std::vector<PersistedTeamFinanceState> SqliteGameStateRepository::loadTeamFinanceStates() const {
    std::vector<PersistedTeamFinanceState> states;
    if (!tableExists(database, "runtime_team_finances")) {
        return states;
    }
    if (!columnExists(database, "runtime_team_finances", "financial_strategy")) {
        throw std::runtime_error(
            "runtime_team_finances is missing financial_strategy; clear incompatible development saves or start a new game");
    }
    if (!columnExists(database, "runtime_team_finances", "financial_health")) {
        throw std::runtime_error(
            "runtime_team_finances is missing financial_health; clear incompatible development saves or start a new game");
    }

    SqliteStatement statement = database.prepare(
        "SELECT league_id, team_id, total_budget, transfer_budget, wage_budget, financial_strategy, financial_health "
        "FROM runtime_team_finances ORDER BY league_id, team_id");
    while (statement.stepRow()) {
        PersistedTeamFinanceState state;
        state.leagueId = static_cast<LeagueId>(statement.columnInt(0));
        state.teamId = static_cast<TeamId>(statement.columnInt(1));
        state.totalBudget = static_cast<Money>(statement.columnInt64(2));
        state.transferBudget = static_cast<Money>(statement.columnInt64(3));
        state.wageBudget = static_cast<Money>(statement.columnInt64(4));
        const std::string strategyCode = statement.columnText(5);
        const std::optional<ClubFinancialStrategy> strategy = clubFinancialStrategyFromStableCode(strategyCode);
        if (!strategy.has_value()) {
            throw std::runtime_error("runtime team finance has invalid financial_strategy stable code: " + strategyCode);
        }
        state.financialStrategy = *strategy;
        const std::string healthCode = statement.columnText(6);
        const std::optional<ClubFinancialHealth> health = clubFinancialHealthFromStableCode(healthCode);
        if (!health.has_value()) {
            throw std::runtime_error("runtime team finance has invalid financial_health stable code: " + healthCode);
        }
        state.financialHealth = *health;
        states.push_back(state);
    }
    return states;
}

std::vector<PersistedPlayerRosterState> SqliteGameStateRepository::loadPlayerRosterStates() const {
    std::vector<PersistedPlayerRosterState> states;
    if (!tableExists(database, "runtime_player_roster_state")) {
        return states;
    }

    SqliteStatement statement = database.prepare(
        "SELECT player_id, league_id, team_id, wage, contract_years, current_season_year "
        "FROM runtime_player_roster_state ORDER BY player_id");
    while (statement.stepRow()) {
        PersistedPlayerRosterState state;
        state.playerId = static_cast<PlayerId>(statement.columnInt(0));
        state.leagueId = static_cast<LeagueId>(statement.columnInt(1));
        state.teamId = static_cast<TeamId>(statement.columnInt(2));
        if (!statement.columnIsNull(3)) {
            state.wage = static_cast<Money>(statement.columnInt64(3));
        }
        if (!statement.columnIsNull(4)) {
            state.contractYears = statement.columnInt(4);
        }
        state.currentSeasonYear = statement.columnIsNull(5) ? -1 : statement.columnInt(5);
        states.push_back(state);
    }
    return states;
}

std::vector<PersistedFreeAgentState> SqliteGameStateRepository::loadFreeAgentStates() const {
    std::vector<PersistedFreeAgentState> states;
    if (!tableExists(database, "runtime_free_agents")) {
        return states;
    }

    SqliteStatement statement = database.prepare(
        "SELECT player_id, previous_league_id, previous_team_id, became_free_agent_date, "
        "wage, contract_years, current_season_year "
        "FROM runtime_free_agents ORDER BY player_id");
    while (statement.stepRow()) {
        PersistedFreeAgentState state;
        state.playerId = static_cast<PlayerId>(statement.columnInt(0));
        if (!statement.columnIsNull(1)) {
            state.previousLeagueId = static_cast<LeagueId>(statement.columnInt(1));
        }
        if (!statement.columnIsNull(2)) {
            state.previousTeamId = static_cast<TeamId>(statement.columnInt(2));
        }
        if (!statement.columnIsNull(3)) {
            state.becameFreeAgentDate = parseDate(statement.columnText(3));
        }
        if (!statement.columnIsNull(4)) {
            state.wage = static_cast<Money>(statement.columnInt64(4));
        }
        if (!statement.columnIsNull(5)) {
            state.contractYears = statement.columnInt(5);
        }
        if (!statement.columnIsNull(6)) {
            state.currentSeasonYear = statement.columnInt(6);
        }
        states.push_back(state);
    }
    return states;
}

std::vector<PersistedTransferOfferState> SqliteGameStateRepository::loadTransferOfferStates() const {
    std::vector<PersistedTransferOfferState> transferOffers;
    if (!tableExists(database, "runtime_transfer_offers")) {
        return transferOffers;
    }

    SqliteStatement statement = database.prepare(
        "SELECT offer_id, created_at, last_valid_date, expiry_policy, "
        "seller_league_id, seller_team_id, buyer_league_id, buyer_team_id, "
        "player_id, fee, status, resolution "
        "FROM runtime_transfer_offers ORDER BY offer_id");
    while (statement.stepRow()) {
        transferOffers.push_back(readTransferOfferRow(statement));
    }
    return transferOffers;
}

PersistedSaveSettings SqliteGameStateRepository::loadSaveSettings() const {
    PersistedSaveSettings settings;
    if (!tableExists(database, "runtime_save_settings")) {
        return settings;
    }

    SqliteStatement statement = database.prepare(
        "SELECT auto_save_frequency, last_auto_save_date FROM runtime_save_settings WHERE id = ?");
    statement.bindInt(1, GameStateRowId);
    if (!statement.stepRow()) {
        return settings;
    }

    const std::optional<AutoSaveFrequency> frequency =
        autoSaveFrequencyFromStableCode(statement.columnText(0));
    settings.autoSaveFrequency = frequency.value_or(AutoSaveFrequency::Weekly);
    if (!statement.columnIsNull(1)) {
        settings.lastAutoSaveDate = parseDate(statement.columnText(1));
    }
    return settings;
}

void SqliteGameStateRepository::saveSaveSettings(const PersistedSaveSettings& settings) const {
    SqliteStatement statement = database.prepare(
        "INSERT INTO runtime_save_settings (id, auto_save_frequency, last_auto_save_date) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "auto_save_frequency = excluded.auto_save_frequency, "
        "last_auto_save_date = excluded.last_auto_save_date");
    statement.bindInt(1, GameStateRowId);
    statement.bindText(2, std::string(toStableCode(settings.autoSaveFrequency)));
    if (settings.lastAutoSaveDate.has_value()) {
        statement.bindText(3, dateToIsoString(*settings.lastAutoSaveDate));
    }
    else {
        statement.bindNull(3);
    }
    statement.stepDone();
}

void SqliteGameStateRepository::saveRuntimeState(
    const Date& currentDate,
    int currentState,
    const std::vector<PersistedLeagueRuntimeState>& leagueStates,
    const std::vector<PersistedFixtureState>& fixtures,
    const std::vector<MatchReport>& reports,
    const std::vector<PersistedTeamSheetState>& teamSheetStates,
    const std::vector<PersistedPlayerRuntimeState>& playerStates,
    const std::vector<PersistedPlayerAttributesState>& playerAttributesStates,
    const std::vector<PersistedTeamFinanceState>& teamFinances,
    const std::vector<PersistedPlayerRosterState>& playerRosterStates,
    const std::vector<PersistedFreeAgentState>& freeAgentStates,
    const std::vector<PersistedTransferOfferState>& transferOffers) const {
    database.execute("BEGIN TRANSACTION;");

    try {
        database.execute("DELETE FROM runtime_match_events;");
        database.execute("DELETE FROM runtime_match_player_reports;");
        database.execute("DELETE FROM runtime_match_reports;");
        database.execute("DELETE FROM runtime_fixtures;");
        database.execute("DELETE FROM runtime_team_sheet_substitutes;");
        database.execute("DELETE FROM runtime_team_sheet_starters;");
        database.execute("DELETE FROM runtime_team_sheets;");
        database.execute("DELETE FROM runtime_transfer_offers;");
        database.execute("DELETE FROM runtime_free_agents;");
        database.execute("DELETE FROM runtime_player_roster_state;");
        database.execute("DELETE FROM runtime_team_finances;");
        database.execute("DELETE FROM league_runtime_state;");
        database.execute("DELETE FROM runtime_player_attributes;");
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
                "away_coach_id, away_formation, away_starting_player_ids, "
                "home_shots, home_shots_on_target, home_passes_attempted, home_passes_completed, "
                "home_tackles_attempted, home_tackles_won, home_interceptions, home_fouls, home_corners, "
                "home_yellow_cards, home_red_cards, home_possession_share, home_expected_goals, "
                "away_shots, away_shots_on_target, away_passes_attempted, away_passes_completed, "
                "away_tackles_attempted, away_tackles_won, away_interceptions, away_fouls, away_corners, "
                "away_yellow_cards, away_red_cards, away_possession_share, away_expected_goals"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
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
            statement.bindInt(16, report.homeStats.shots);
            statement.bindInt(17, report.homeStats.shotsOnTarget);
            statement.bindInt(18, report.homeStats.passesAttempted);
            statement.bindInt(19, report.homeStats.passesCompleted);
            statement.bindInt(20, report.homeStats.tacklesAttempted);
            statement.bindInt(21, report.homeStats.tacklesWon);
            statement.bindInt(22, report.homeStats.interceptions);
            statement.bindInt(23, report.homeStats.fouls);
            statement.bindInt(24, report.homeStats.corners);
            statement.bindInt(25, report.homeStats.yellowCards);
            statement.bindInt(26, report.homeStats.redCards);
            statement.bindDouble(27, report.homeStats.possessionShare);
            statement.bindDouble(28, report.homeStats.expectedGoals);
            statement.bindInt(29, report.awayStats.shots);
            statement.bindInt(30, report.awayStats.shotsOnTarget);
            statement.bindInt(31, report.awayStats.passesAttempted);
            statement.bindInt(32, report.awayStats.passesCompleted);
            statement.bindInt(33, report.awayStats.tacklesAttempted);
            statement.bindInt(34, report.awayStats.tacklesWon);
            statement.bindInt(35, report.awayStats.interceptions);
            statement.bindInt(36, report.awayStats.fouls);
            statement.bindInt(37, report.awayStats.corners);
            statement.bindInt(38, report.awayStats.yellowCards);
            statement.bindInt(39, report.awayStats.redCards);
            statement.bindDouble(40, report.awayStats.possessionShare);
            statement.bindDouble(41, report.awayStats.expectedGoals);
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

        for (const PersistedTeamSheetState& state : teamSheetStates) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_team_sheets ("
                "league_id, team_id, formation, mentality, tempo, width, defensive_line, "
                "pressing_intensity, marking_style, passing_directness"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt(1, static_cast<int>(state.leagueId));
            statement.bindInt(2, static_cast<int>(state.teamSheet.teamId));
            statement.bindText(3, toStableCode(state.teamSheet.formation));
            statement.bindText(4, std::string(toStableCode(state.teamSheet.tacticalSetup.mentality)));
            statement.bindText(5, std::string(toStableCode(state.teamSheet.tacticalSetup.tempo)));
            statement.bindText(6, std::string(toStableCode(state.teamSheet.tacticalSetup.width)));
            statement.bindText(7, std::string(toStableCode(state.teamSheet.tacticalSetup.defensiveLine)));
            statement.bindText(8, std::string(toStableCode(state.teamSheet.tacticalSetup.pressingIntensity)));
            statement.bindText(9, std::string(toStableCode(state.teamSheet.tacticalSetup.markingStyle)));
            statement.bindText(10, std::string(toStableCode(state.teamSheet.tacticalSetup.passingDirectness)));
            statement.stepDone();

            for (const TeamSheetSlotAssignment& assignment : state.teamSheet.startingAssignments) {
                SqliteStatement starterStatement = database.prepare(
                    "INSERT INTO runtime_team_sheet_starters ("
                    "league_id, team_id, slot_index, slot_role, player_id"
                    ") VALUES (?, ?, ?, ?, ?)");
                starterStatement.bindInt(1, static_cast<int>(state.leagueId));
                starterStatement.bindInt(2, static_cast<int>(state.teamSheet.teamId));
                starterStatement.bindInt(3, static_cast<int>(assignment.slotIndex));
                starterStatement.bindText(4, toStableCode(assignment.slotRole));
                starterStatement.bindInt(5, static_cast<int>(assignment.playerId));
                starterStatement.stepDone();
            }

            for (std::size_t substituteIndex = 0; substituteIndex < state.teamSheet.substitutePlayerIds.size(); ++substituteIndex) {
                SqliteStatement substituteStatement = database.prepare(
                    "INSERT INTO runtime_team_sheet_substitutes ("
                    "league_id, team_id, substitute_order, player_id"
                    ") VALUES (?, ?, ?, ?)");
                substituteStatement.bindInt(1, static_cast<int>(state.leagueId));
                substituteStatement.bindInt(2, static_cast<int>(state.teamSheet.teamId));
                substituteStatement.bindInt(3, static_cast<int>(substituteIndex));
                substituteStatement.bindInt(4, static_cast<int>(state.teamSheet.substitutePlayerIds[substituteIndex]));
                substituteStatement.stepDone();
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

        for (const PersistedPlayerAttributesState& state : playerAttributesStates) {
            validatePlayerAttributes(state.attributes);
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_player_attributes ("
                "player_id, "
                "shooting, passing, first_touch, technique, tackling, dribbling, crossing, marking, heading, set_pieces, "
                "decisions, vision, positioning, off_the_ball, composure, concentration, work_rate, teamwork, leadership, aggression, "
                "pace, acceleration, stamina, strength, agility, jumping_reach, "
                "shot_stopping, handling, aerial_ability, one_on_ones, distribution"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt(1, static_cast<int>(state.playerId));
            statement.bindInt(2, state.attributes.technical.shooting);
            statement.bindInt(3, state.attributes.technical.passing);
            statement.bindInt(4, state.attributes.technical.firstTouch);
            statement.bindInt(5, state.attributes.technical.technique);
            statement.bindInt(6, state.attributes.technical.tackling);
            statement.bindInt(7, state.attributes.technical.dribbling);
            statement.bindInt(8, state.attributes.technical.crossing);
            statement.bindInt(9, state.attributes.technical.marking);
            statement.bindInt(10, state.attributes.technical.heading);
            statement.bindInt(11, state.attributes.technical.setPieces);
            statement.bindInt(12, state.attributes.mental.decisions);
            statement.bindInt(13, state.attributes.mental.vision);
            statement.bindInt(14, state.attributes.mental.positioning);
            statement.bindInt(15, state.attributes.mental.offTheBall);
            statement.bindInt(16, state.attributes.mental.composure);
            statement.bindInt(17, state.attributes.mental.concentration);
            statement.bindInt(18, state.attributes.mental.workRate);
            statement.bindInt(19, state.attributes.mental.teamwork);
            statement.bindInt(20, state.attributes.mental.leadership);
            statement.bindInt(21, state.attributes.mental.aggression);
            statement.bindInt(22, state.attributes.physical.pace);
            statement.bindInt(23, state.attributes.physical.acceleration);
            statement.bindInt(24, state.attributes.physical.stamina);
            statement.bindInt(25, state.attributes.physical.strength);
            statement.bindInt(26, state.attributes.physical.agility);
            statement.bindInt(27, state.attributes.physical.jumpingReach);
            statement.bindInt(28, state.attributes.goalkeeper.shotStopping);
            statement.bindInt(29, state.attributes.goalkeeper.handling);
            statement.bindInt(30, state.attributes.goalkeeper.aerialAbility);
            statement.bindInt(31, state.attributes.goalkeeper.oneOnOnes);
            statement.bindInt(32, state.attributes.goalkeeper.distribution);
            statement.stepDone();
        }

        for (const PersistedTeamFinanceState& state : teamFinances) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_team_finances ("
                "league_id, team_id, total_budget, transfer_budget, wage_budget, financial_strategy, financial_health"
                ") VALUES (?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt(1, static_cast<int>(state.leagueId));
            statement.bindInt(2, static_cast<int>(state.teamId));
            statement.bindInt64(3, static_cast<std::int64_t>(state.totalBudget));
            statement.bindInt64(4, static_cast<std::int64_t>(state.transferBudget));
            statement.bindInt64(5, static_cast<std::int64_t>(state.wageBudget));
            statement.bindText(6, toStableCode(state.financialStrategy));
            statement.bindText(7, toStableCode(state.financialHealth));
            statement.stepDone();
        }

        for (const PersistedPlayerRosterState& state : playerRosterStates) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_player_roster_state ("
                "player_id, league_id, team_id, wage, contract_years, current_season_year"
                ") VALUES (?, ?, ?, ?, ?, ?)");
            statement.bindInt(1, static_cast<int>(state.playerId));
            statement.bindInt(2, static_cast<int>(state.leagueId));
            statement.bindInt(3, static_cast<int>(state.teamId));
            if (state.wage.has_value()) {
                statement.bindInt64(4, static_cast<std::int64_t>(*state.wage));
            }
            else {
                statement.bindNull(4);
            }
            if (state.contractYears.has_value()) {
                statement.bindInt(5, *state.contractYears);
            }
            else {
                statement.bindNull(5);
            }
            if (state.currentSeasonYear >= 0) {
                statement.bindInt(6, state.currentSeasonYear);
            }
            else {
                statement.bindNull(6);
            }
            statement.stepDone();
        }

        for (const PersistedFreeAgentState& state : freeAgentStates) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_free_agents ("
                "player_id, previous_league_id, previous_team_id, became_free_agent_date, "
                "wage, contract_years, current_season_year"
                ") VALUES (?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt(1, static_cast<int>(state.playerId));
            if (state.previousLeagueId != 0) {
                statement.bindInt(2, static_cast<int>(state.previousLeagueId));
            }
            else {
                statement.bindNull(2);
            }
            if (state.previousTeamId != 0) {
                statement.bindInt(3, static_cast<int>(state.previousTeamId));
            }
            else {
                statement.bindNull(3);
            }
            if (state.becameFreeAgentDate.has_value()) {
                statement.bindText(4, dateToIsoString(*state.becameFreeAgentDate));
            }
            else {
                statement.bindNull(4);
            }
            if (state.wage.has_value()) {
                statement.bindInt64(5, static_cast<std::int64_t>(*state.wage));
            }
            else {
                statement.bindNull(5);
            }
            if (state.contractYears.has_value()) {
                statement.bindInt(6, *state.contractYears);
            }
            else {
                statement.bindNull(6);
            }
            if (state.currentSeasonYear.has_value()) {
                statement.bindInt(7, *state.currentSeasonYear);
            }
            else {
                statement.bindNull(7);
            }
            statement.stepDone();
        }

        for (const PersistedTransferOfferState& state : transferOffers) {
            SqliteStatement statement = database.prepare(
                "INSERT INTO runtime_transfer_offers ("
                "offer_id, created_at, last_valid_date, expiry_policy, "
                "seller_league_id, seller_team_id, buyer_league_id, buyer_team_id, "
                "player_id, fee, status, resolution"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt64(1, static_cast<std::int64_t>(state.offerId));
            statement.bindText(2, dateToIsoString(state.createdAt));
            statement.bindText(3, dateToIsoString(state.lastValidDate));
            statement.bindText(4, toStableCode(state.expiryPolicy));
            statement.bindInt(5, static_cast<int>(state.sellerLeagueId));
            statement.bindInt(6, static_cast<int>(state.sellerTeamId));
            statement.bindInt(7, static_cast<int>(state.buyerLeagueId));
            statement.bindInt(8, static_cast<int>(state.buyerTeamId));
            statement.bindInt(9, static_cast<int>(state.playerId));
            statement.bindInt(10, state.fee);
            statement.bindText(11, toStableCode(state.status));
            if (state.resolution.has_value()) {
                statement.bindText(12, toStableCode(*state.resolution));
            }
            else {
                statement.bindNull(12);
            }
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
