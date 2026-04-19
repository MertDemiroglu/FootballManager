#include "fm/data/WorldBootstrapLoader.h"

#include "fm/core/World.h"
#include "fm/competition/League.h"
#include "fm/data/SqliteBootstrapRepository.h"
#include "fm/roster/FootballerFactory.h"
#include "fm/roster/HeadCoach.h"
#include "fm/roster/Team.h"

#include <memory>
#include <stdexcept>
#include <string>

WorldBootstrapLoader::WorldBootstrapLoader(SqliteBootstrapRepository& repository)
    : repository(repository) {
}

void WorldBootstrapLoader::load(World& world, const LeagueRules& rules, const SeasonPlan& seasonPlan) const {
    const std::vector<LeagueSeedData> leagues = repository.loadLeagues();

    for (const LeagueSeedData& leagueSeed : leagues) {
        League league(leagueSeed.name, leagueSeed.id);

        for (const TeamSeedData& teamSeed : leagueSeed.teams) {
            auto team = std::make_unique<Team>(teamSeed.id, teamSeed.name);

            if (teamSeed.leagueId != leagueSeed.id) {
                throw std::logic_error("team seed league id mismatch for team id: " + std::to_string(teamSeed.id));
            }

            team->setHeadCoach(HeadCoach(teamSeed.coach.id, teamSeed.coach.name, teamSeed.coach.preferredFormation));
            team->setBudgetSnapshot(teamSeed.totalBudget, teamSeed.transferBudget, teamSeed.wageBudget);

            for (const PlayerSeedData& playerSeed : teamSeed.players) {
                if (playerSeed.teamId != teamSeed.id) {
                    throw std::logic_error("player team id mismatch for player id: " + std::to_string(playerSeed.id));
                }

                auto player = FootballerFactory::create(
                    playerSeed.name,
                    playerSeed.age,
                    playerSeed.position,
                    teamSeed.name,
                    playerSeed.s1,
                    playerSeed.s2,
                    playerSeed.s3,
                    playerSeed.s4,
                    playerSeed.s5);

                if (!player) {
                    throw std::runtime_error("failed to create footballer for seed player: " + playerSeed.name);
                }

                player->setIdForBootstrap(playerSeed.id);
                player->setTeam(teamSeed.id);
                player->signContract(playerSeed.wage, playerSeed.contractYears);
                team->addPlayer(std::move(player));
            }

            league.addTeam(std::move(team));
        }

        world.addLeagueContext(std::move(league), rules, seasonPlan);
    }
}
