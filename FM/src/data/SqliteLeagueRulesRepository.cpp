#include "fm/data/SqliteLeagueRulesRepository.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    struct MonthDayRule {
        int yearOffset = 0;
        Month month = Month::January;
        int day = 1;
    };

    struct KickoffRuleConfig {
        std::string rule;
        Month month = Month::January;
        int day = 0;
        int weekday = 0;
        int weekOfMonth = 0;
    };

    struct TransferWindowRule {
        MonthDayRule start;
        MonthDayRule endExclusive;
    };

    Month parseMonth(int value, const std::string& context) {
        if (value < static_cast<int>(Month::January) || value > static_cast<int>(Month::December)) {
            throw std::runtime_error(context + " has invalid month value: " + std::to_string(value));
        }
        return static_cast<Month>(value);
    }

    void validateDay(Month month, int day, const std::string& context) {
        if (day < 1 || day > Date::daysInMonth(2024, month)) {
            throw std::runtime_error(context + " has invalid day value: " + std::to_string(day));
        }
    }

    Date makeDate(int seasonYear, const MonthDayRule& rule) {
        return Date(seasonYear + rule.yearOffset, rule.month, rule.day);
    }

    Date makeNthWeekdayDate(int seasonYear, const KickoffRuleConfig& config) {
        if (config.weekday < 0 || config.weekday > 6) {
            throw std::runtime_error("league kickoff rule has invalid weekday: " + std::to_string(config.weekday));
        }
        if (config.weekOfMonth < 1 || config.weekOfMonth > 5) {
            throw std::runtime_error("league kickoff rule has invalid week of month: " + std::to_string(config.weekOfMonth));
        }

        Date date(seasonYear, config.month, 1);
        while (date.getDayOfWeek() != config.weekday) {
            date.advanceDay();
        }

        for (int week = 1; week < config.weekOfMonth; ++week) {
            for (int day = 0; day < 7; ++day) {
                date.advanceDay();
            }
        }

        return date;
    }

    MonthDayRule makeMonthDayRule(int yearOffset, int monthValue, int day, const std::string& context) {
        MonthDayRule rule;
        rule.yearOffset = yearOffset;
        rule.month = parseMonth(monthValue, context);
        rule.day = day;
        validateDay(rule.month, rule.day, context);
        return rule;
    }

    TransferWindowRule loadTransferWindowRule(
        const SqliteDatabase& database,
        LeagueId leagueId,
        const std::string& windowCode) {
        SqliteStatement statement = database.prepare(
            "SELECT start_year_offset, start_month, start_day, "
            "end_year_offset, end_month, end_day "
            "FROM league_transfer_windows "
            "WHERE league_id = ? AND window_code = ?;");
        statement.bindInt(1, static_cast<int>(leagueId));
        statement.bindText(2, windowCode);

        if (!statement.stepRow()) {
            throw std::runtime_error("missing league transfer window '" + windowCode + "' for league id " + std::to_string(leagueId));
        }

        TransferWindowRule rule;
        rule.start = makeMonthDayRule(
            statement.columnInt(0),
            statement.columnInt(1),
            statement.columnInt(2),
            "league transfer window '" + windowCode + "' start");
        rule.endExclusive = makeMonthDayRule(
            statement.columnInt(3),
            statement.columnInt(4),
            statement.columnInt(5),
            "league transfer window '" + windowCode + "' end");

        if (statement.stepRow()) {
            throw std::runtime_error("duplicate league transfer window '" + windowCode + "' for league id " + std::to_string(leagueId));
        }

        return rule;
    }

    std::vector<int> loadDistributionOffsets(const SqliteDatabase& database, LeagueId leagueId) {
        SqliteStatement statement = database.prepare(
            "SELECT offset_index, offset_days "
            "FROM league_matchday_distribution_offsets "
            "WHERE league_id = ? "
            "ORDER BY offset_index ASC;");
        statement.bindInt(1, static_cast<int>(leagueId));

        std::vector<int> offsets;
        int expectedIndex = 0;
        while (statement.stepRow()) {
            const int offsetIndex = statement.columnInt(0);
            if (offsetIndex != expectedIndex) {
                throw std::runtime_error("league matchday distribution offsets must be zero-based and contiguous for league id " + std::to_string(leagueId));
            }
            offsets.push_back(statement.columnInt(1));
            ++expectedIndex;
        }

        if (offsets.empty()) {
            throw std::runtime_error("missing league matchday distribution offsets for league id " + std::to_string(leagueId));
        }

        return offsets;
    }
}

SqliteLeagueRulesRepository::SqliteLeagueRulesRepository(const std::string& dbPath)
    : database(dbPath) {
    ensureSchema();
}

void SqliteLeagueRulesRepository::ensureSchema() const {
    database.execute(
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS league_rules ("
        "league_id INTEGER PRIMARY KEY,"
        "league_code TEXT NOT NULL,"
        "league_name TEXT NOT NULL,"
        "team_count INTEGER NOT NULL,"
        "matchdays_per_season INTEGER NOT NULL,"
        "first_half_rounds INTEGER NOT NULL,"
        "preseason_start_month INTEGER NOT NULL,"
        "preseason_start_day INTEGER NOT NULL,"
        "next_preseason_year_offset INTEGER NOT NULL,"
        "next_preseason_start_month INTEGER NOT NULL,"
        "next_preseason_start_day INTEGER NOT NULL,"
        "kickoff_rule TEXT NOT NULL,"
        "kickoff_month INTEGER NOT NULL,"
        "kickoff_day INTEGER,"
        "kickoff_weekday INTEGER,"
        "kickoff_week_of_month INTEGER,"
        "winter_break_enabled INTEGER NOT NULL,"
        "winter_break_length_days INTEGER NOT NULL,"
        "winter_break_after_round_index INTEGER NOT NULL,"
        "match_spacing_days INTEGER NOT NULL,"
        "FOREIGN KEY (league_id) REFERENCES leagues(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS league_transfer_windows ("
        "league_id INTEGER NOT NULL,"
        "window_code TEXT NOT NULL,"
        "start_year_offset INTEGER NOT NULL,"
        "start_month INTEGER NOT NULL,"
        "start_day INTEGER NOT NULL,"
        "end_year_offset INTEGER NOT NULL,"
        "end_month INTEGER NOT NULL,"
        "end_day INTEGER NOT NULL,"
        "PRIMARY KEY (league_id, window_code),"
        "FOREIGN KEY (league_id) REFERENCES leagues(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS league_matchday_distribution_offsets ("
        "league_id INTEGER NOT NULL,"
        "offset_index INTEGER NOT NULL,"
        "offset_days INTEGER NOT NULL,"
        "PRIMARY KEY (league_id, offset_index),"
        "FOREIGN KEY (league_id) REFERENCES leagues(id)"
        ");"
        "PRAGMA user_version = 11;");
}

LeagueRules SqliteLeagueRulesRepository::loadLeagueRules(LeagueId leagueId) const {
    ensureSchema();

    SqliteStatement statement = database.prepare(
        "SELECT league_code, league_name, team_count, matchdays_per_season, first_half_rounds, "
        "preseason_start_month, preseason_start_day, next_preseason_year_offset, "
        "next_preseason_start_month, next_preseason_start_day, kickoff_rule, kickoff_month, "
        "kickoff_day, kickoff_weekday, kickoff_week_of_month, winter_break_enabled, "
        "winter_break_length_days, winter_break_after_round_index, match_spacing_days "
        "FROM league_rules WHERE league_id = ?;");
    statement.bindInt(1, static_cast<int>(leagueId));

    if (!statement.stepRow()) {
        throw std::runtime_error("missing SQL league_rules row for league id " + std::to_string(leagueId));
    }

    LeagueRules rules;
    rules.leagueId = statement.columnText(0);
    rules.leagueName = statement.columnText(1);
    rules.teamCount = statement.columnInt(2);
    rules.matchdaysPerSeason = statement.columnInt(3);
    rules.firstHalfRounds = statement.columnInt(4);

    if (rules.leagueId.empty() || rules.leagueName.empty()) {
        throw std::runtime_error("league_rules has empty league code or name for league id " + std::to_string(leagueId));
    }
    if (rules.teamCount <= 0 || rules.matchdaysPerSeason <= 0 || rules.firstHalfRounds <= 0) {
        throw std::runtime_error("league_rules has invalid season counts for league id " + std::to_string(leagueId));
    }

    const MonthDayRule preseasonStart = makeMonthDayRule(
        0,
        statement.columnInt(5),
        statement.columnInt(6),
        "league preseason start");
    const MonthDayRule nextPreseasonStart = makeMonthDayRule(
        statement.columnInt(7),
        statement.columnInt(8),
        statement.columnInt(9),
        "league next preseason start");

    KickoffRuleConfig kickoff;
    kickoff.rule = statement.columnText(10);
    kickoff.month = parseMonth(statement.columnInt(11), "league kickoff");
    kickoff.day = statement.columnInt(12);
    kickoff.weekday = statement.columnInt(13);
    kickoff.weekOfMonth = statement.columnInt(14);

    if (kickoff.rule == "fixed_date") {
        validateDay(kickoff.month, kickoff.day, "league kickoff fixed date");
    } else if (kickoff.rule != "nth_weekday_of_month") {
        throw std::runtime_error("unsupported league kickoff rule '" + kickoff.rule + "' for league id " + std::to_string(leagueId));
    }

    const TransferWindowRule summerWindow = loadTransferWindowRule(database, leagueId, "summer");
    const TransferWindowRule winterWindow = loadTransferWindowRule(database, leagueId, "winter");

    // TODO: Move LeagueRules to fully value-based rule configs once callers no longer require function fields.
    rules.preseasonStart = [preseasonStart](int seasonYear) {
        return makeDate(seasonYear, preseasonStart);
        };
    rules.nextPreseasonStart = [nextPreseasonStart](int seasonYear) {
        return makeDate(seasonYear, nextPreseasonStart);
        };
    rules.kickoffDate = [kickoff](int seasonYear) {
        if (kickoff.rule == "fixed_date") {
            return Date(seasonYear, kickoff.month, kickoff.day);
        }
        return makeNthWeekdayDate(seasonYear, kickoff);
        };
    rules.summerWindow = [summerWindow](int seasonYear) {
        return TransferWindow{ makeDate(seasonYear, summerWindow.start), makeDate(seasonYear, summerWindow.endExclusive) };
        };
    rules.winterWindow = [winterWindow](int seasonYear) {
        return TransferWindow{ makeDate(seasonYear, winterWindow.start), makeDate(seasonYear, winterWindow.endExclusive) };
        };

    rules.winterBreakEnabled = statement.columnInt(15) != 0;
    rules.winterBreakLengthDays = statement.columnInt(16);
    rules.winterBreakAfterRoundIndex = statement.columnInt(17);
    rules.matchSpacingDays = statement.columnInt(18);
    rules.matchdayDistributionOffsets = loadDistributionOffsets(database, leagueId);

    if (rules.winterBreakLengthDays < 0 || rules.winterBreakAfterRoundIndex < 0 || rules.matchSpacingDays <= 0) {
        throw std::runtime_error("league_rules has invalid break or spacing values for league id " + std::to_string(leagueId));
    }

    if (statement.stepRow()) {
        throw std::runtime_error("duplicate SQL league_rules row for league id " + std::to_string(leagueId));
    }

    return rules;
}

std::unordered_map<LeagueId, LeagueRules> SqliteLeagueRulesRepository::loadAllLeagueRules() const {
    ensureSchema();

    SqliteStatement statement = database.prepare("SELECT league_id FROM league_rules ORDER BY league_id ASC;");
    std::vector<LeagueId> leagueIds;
    while (statement.stepRow()) {
        leagueIds.push_back(static_cast<LeagueId>(statement.columnInt(0)));
    }

    if (leagueIds.empty()) {
        throw std::runtime_error("no SQL league_rules rows found");
    }

    std::unordered_map<LeagueId, LeagueRules> rulesByLeague;
    for (LeagueId leagueId : leagueIds) {
        rulesByLeague.emplace(leagueId, loadLeagueRules(leagueId));
    }

    return rulesByLeague;
}
