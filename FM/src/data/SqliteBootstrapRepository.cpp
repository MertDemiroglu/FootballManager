#include "fm/data/SqliteBootstrapRepository.h"

#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <utility>

namespace {
    constexpr const char* DefaultPrimaryColor = "#22c55e";
    constexpr const char* DefaultSecondaryColor = "#0f172a";

    std::string fallbackColor(std::string color, const char* fallback) {
        return color.empty() ? std::string(fallback) : color;
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

    void ensurePlayerAttributesTable(const SqliteDatabase& database) {
        database.execute(
            "CREATE TABLE IF NOT EXISTS player_attributes ("
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
            "distribution INTEGER NOT NULL,"
            "FOREIGN KEY (player_id) REFERENCES players(id)"
            ");");
    }

    FormationId parseFormationId(int rawValue) {
        switch (rawValue) {
        case 0:
            return FormationId::FourFourTwo;
        case 1:
            return FormationId::FourThreeThree;
        case 2:
            return FormationId::ThreeFiveTwo;
        default:
            throw std::runtime_error("unsupported formation id value in seed data: " + std::to_string(rawValue));
        }
    }
}

SqliteBootstrapRepository::SqliteBootstrapRepository(const std::string& databasePath)
    : database(databasePath) {
    ensurePlayerAttributesTable(database);
}

std::vector<LeagueSeedData> SqliteBootstrapRepository::loadLeagues() const {
    std::vector<LeagueSeedData> leagues;
    std::unordered_map<LeagueId, std::size_t> leagueIndexById;
    std::unordered_set<LeagueId> leagueIds;

    {
        SqliteStatement leagueStmt = database.prepare("SELECT id, name FROM leagues ORDER BY id");
        while (leagueStmt.stepRow()) {
            LeagueSeedData league;
            league.id = static_cast<LeagueId>(leagueStmt.columnInt(0));
            league.name = leagueStmt.columnText(1);

            if (league.id == 0) {
                throw std::runtime_error("league id cannot be zero in seed data");
            }
            if (league.name.empty()) {
                throw std::runtime_error("league name cannot be empty in seed data");
            }

            if (!leagueIds.emplace(league.id).second) {
                throw std::logic_error("duplicate league id in seed data: " + std::to_string(league.id));
            }

            leagueIndexById.emplace(league.id, leagues.size());
            leagues.push_back(std::move(league));
        }
    }

    std::unordered_map<TeamId, std::pair<std::size_t, std::size_t>> teamIndexById;
    std::unordered_set<TeamId> teamIds;

    {
        const char* teamSql =
            "SELECT t.id, t.league_id, t.name, t.transfer_budget, t.wage_budget, t.total_budget, "
            "t.primary_color, t.secondary_color, c.id, c.name, c.preferred_formation "
            "FROM teams t "
            "JOIN coaches c ON c.id = t.coach_id "
            "ORDER BY t.id";

        SqliteStatement teamStmt = database.prepare(teamSql);
        while (teamStmt.stepRow()) {
            const TeamId teamId = static_cast<TeamId>(teamStmt.columnInt(0));
            const LeagueId leagueId = static_cast<LeagueId>(teamStmt.columnInt(1));

            auto leagueIt = leagueIndexById.find(leagueId);
            if (leagueIt == leagueIndexById.end()) {
                throw std::runtime_error("team references unknown league id: " + std::to_string(leagueId));
            }

            TeamSeedData team;
            team.id = teamId;
            team.leagueId = leagueId;
            team.name = teamStmt.columnText(2);
            team.transferBudget = static_cast<Money>(teamStmt.columnInt64(3));
            team.wageBudget = static_cast<Money>(teamStmt.columnInt64(4));
            team.totalBudget = static_cast<Money>(teamStmt.columnInt64(5));
            team.primaryColor = fallbackColor(teamStmt.columnText(6), DefaultPrimaryColor);
            team.secondaryColor = fallbackColor(teamStmt.columnText(7), DefaultSecondaryColor);
            team.coach.id = static_cast<CoachId>(teamStmt.columnInt(8));
            team.coach.name = teamStmt.columnText(9);
            team.coach.preferredFormation = parseFormationId(teamStmt.columnInt(10));

            if (team.id == 0) {
                throw std::runtime_error("team id cannot be zero in seed data");
            }
            if (team.name.empty()) {
                throw std::runtime_error("team name cannot be empty in seed data");
            }
            if (team.coach.id == 0) {
                throw std::runtime_error("coach id cannot be zero in seed data");
            }
            if (team.coach.name.empty()) {
                throw std::runtime_error("coach name cannot be empty in seed data");
            }
            if (team.transferBudget < 0 || team.wageBudget < 0 || team.totalBudget < 0) {
                throw std::runtime_error("team budgets cannot be negative in seed data");
            }
            if (!teamIds.emplace(team.id).second) {
                throw std::logic_error("duplicate team id in seed data: " + std::to_string(team.id));
            }

            LeagueSeedData& league = leagues.at(leagueIt->second);
            teamIndexById.emplace(team.id, std::make_pair(leagueIt->second, league.teams.size()));
            league.teams.push_back(std::move(team));
        }
    }

    std::unordered_set<PlayerId> playerIds;

    {
        const bool hasPlayerAttributesTable = tableExists(database, "player_attributes");
        const char* legacyPlayerSql =
            "SELECT id, team_id, name, position, age, wage, contract_years, s1, s2, s3, s4, s5 "
            "FROM players "
            "ORDER BY id";
        const char* playerWithAttributesSql =
            "SELECT p.id, p.team_id, p.name, p.position, p.age, p.wage, p.contract_years, "
            "p.s1, p.s2, p.s3, p.s4, p.s5, "
            "a.shooting, a.passing, a.first_touch, a.technique, a.tackling, "
            "a.dribbling, a.crossing, a.marking, a.heading, a.set_pieces, "
            "a.decisions, a.vision, a.positioning, a.off_the_ball, a.composure, "
            "a.concentration, a.work_rate, a.teamwork, a.leadership, a.aggression, "
            "a.pace, a.acceleration, a.stamina, a.strength, a.agility, a.jumping_reach, "
            "a.shot_stopping, a.handling, a.aerial_ability, a.one_on_ones, a.distribution "
            "FROM players p "
            "LEFT JOIN player_attributes a ON a.player_id = p.id "
            "ORDER BY p.id";

        SqliteStatement playerStmt = database.prepare(hasPlayerAttributesTable ? playerWithAttributesSql : legacyPlayerSql);
        while (playerStmt.stepRow()) {
            PlayerSeedData player;
            player.id = static_cast<PlayerId>(playerStmt.columnInt(0));
            player.teamId = static_cast<TeamId>(playerStmt.columnInt(1));
            player.name = playerStmt.columnText(2);
            player.position = playerStmt.columnText(3);
            player.age = playerStmt.columnInt(4);
            player.wage = static_cast<Money>(playerStmt.columnInt64(5));
            player.contractYears = playerStmt.columnInt(6);
            player.s1 = playerStmt.columnInt(7);
            player.s2 = playerStmt.columnInt(8);
            player.s3 = playerStmt.columnInt(9);
            player.s4 = playerStmt.columnInt(10);
            player.s5 = playerStmt.columnInt(11);

            if (hasPlayerAttributesTable && !playerStmt.columnIsNull(12)) {
                player.hasExplicitAttributes = true;
                player.attributes.technical.shooting = playerStmt.columnInt(12);
                player.attributes.technical.passing = playerStmt.columnInt(13);
                player.attributes.technical.firstTouch = playerStmt.columnInt(14);
                player.attributes.technical.technique = playerStmt.columnInt(15);
                player.attributes.technical.tackling = playerStmt.columnInt(16);
                player.attributes.technical.dribbling = playerStmt.columnInt(17);
                player.attributes.technical.crossing = playerStmt.columnInt(18);
                player.attributes.technical.marking = playerStmt.columnInt(19);
                player.attributes.technical.heading = playerStmt.columnInt(20);
                player.attributes.technical.setPieces = playerStmt.columnInt(21);
                player.attributes.mental.decisions = playerStmt.columnInt(22);
                player.attributes.mental.vision = playerStmt.columnInt(23);
                player.attributes.mental.positioning = playerStmt.columnInt(24);
                player.attributes.mental.offTheBall = playerStmt.columnInt(25);
                player.attributes.mental.composure = playerStmt.columnInt(26);
                player.attributes.mental.concentration = playerStmt.columnInt(27);
                player.attributes.mental.workRate = playerStmt.columnInt(28);
                player.attributes.mental.teamwork = playerStmt.columnInt(29);
                player.attributes.mental.leadership = playerStmt.columnInt(30);
                player.attributes.mental.aggression = playerStmt.columnInt(31);
                player.attributes.physical.pace = playerStmt.columnInt(32);
                player.attributes.physical.acceleration = playerStmt.columnInt(33);
                player.attributes.physical.stamina = playerStmt.columnInt(34);
                player.attributes.physical.strength = playerStmt.columnInt(35);
                player.attributes.physical.agility = playerStmt.columnInt(36);
                player.attributes.physical.jumpingReach = playerStmt.columnInt(37);
                player.attributes.goalkeeper.shotStopping = playerStmt.columnInt(38);
                player.attributes.goalkeeper.handling = playerStmt.columnInt(39);
                player.attributes.goalkeeper.aerialAbility = playerStmt.columnInt(40);
                player.attributes.goalkeeper.oneOnOnes = playerStmt.columnInt(41);
                player.attributes.goalkeeper.distribution = playerStmt.columnInt(42);
                validatePlayerAttributes(player.attributes);
            }

            if (player.id == 0) {
                throw std::runtime_error("player id cannot be zero in seed data");
            }
            if (player.teamId == 0) {
                throw std::runtime_error("player team id cannot be zero in seed data");
            }
            if (player.name.empty()) {
                throw std::runtime_error("player name cannot be empty in seed data");
            }
            if (player.position.empty()) {
                throw std::runtime_error("player position cannot be empty in seed data");
            }
            if (player.age <= 0) {
                throw std::runtime_error("player age must be positive in seed data for player id: " + std::to_string(player.id));
            }
            if (player.wage < 0) {
                throw std::runtime_error("player wage cannot be negative in seed data for player id: " + std::to_string(player.id));
            }
            if (player.contractYears <= 0) {
                throw std::runtime_error("player contract years must be positive in seed data for player id: " + std::to_string(player.id));
            }
            if (!playerIds.emplace(player.id).second) {
                throw std::logic_error("duplicate player id in seed data: " + std::to_string(player.id));
            }

            auto teamIt = teamIndexById.find(player.teamId);
            if (teamIt == teamIndexById.end()) {
                throw std::runtime_error("player references unknown team id: " + std::to_string(player.teamId));
            }

            LeagueSeedData& league = leagues.at(teamIt->second.first);
            TeamSeedData& team = league.teams.at(teamIt->second.second);
            team.players.push_back(std::move(player));
        }
    }

    return leagues;
}
