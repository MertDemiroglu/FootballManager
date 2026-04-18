#include "fm/data/SqliteBootstrapRepository.h"

#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace {
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
}

std::vector<LeagueSeedData> SqliteBootstrapRepository::loadLeagues() const {
    std::vector<LeagueSeedData> leagues;
    std::unordered_map<LeagueId, std::size_t> leagueIndexById;

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

            leagueIndexById.emplace(league.id, leagues.size());
            leagues.push_back(std::move(league));
        }
    }

    std::unordered_map<TeamId, std::pair<std::size_t, std::size_t>> teamIndexById;

    {
        const char* teamSql =
            "SELECT t.id, t.league_id, t.name, t.transfer_budget, t.wage_budget, t.total_budget, "
            "c.id, c.name, c.preferred_formation "
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
            team.coach.id = static_cast<CoachId>(teamStmt.columnInt(6));
            team.coach.name = teamStmt.columnText(7);
            team.coach.preferredFormation = parseFormationId(teamStmt.columnInt(8));

            if (team.id == 0) {
                throw std::runtime_error("team id cannot be zero in seed data");
            }
            if (team.name.empty()) {
                throw std::runtime_error("team name cannot be empty in seed data");
            }

            LeagueSeedData& league = leagues.at(leagueIt->second);
            teamIndexById.emplace(team.id, std::make_pair(leagueIt->second, league.teams.size()));
            league.teams.push_back(std::move(team));
        }
    }

    {
        const char* playerSql =
            "SELECT id, team_id, name, position, age, wage, contract_years, s1, s2, s3, s4, s5 "
            "FROM players "
            "ORDER BY id";

        SqliteStatement playerStmt = database.prepare(playerSql);
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
