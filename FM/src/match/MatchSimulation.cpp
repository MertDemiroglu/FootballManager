#include"fm/match/MatchSimulation.h"

#include"fm/roster/Team.h"
#include"fm/roster/Formation.h"

#include<algorithm>
#include<cstdint>
#include<random>
#include<stdexcept>
#include<unordered_set>
#include<vector>

namespace {
    std::uint32_t makeMatchSeed(TeamId homeId, TeamId awayId, const Date& date) {
        std::uint32_t seed = 2166136261u;

        auto mix = [&seed](std::uint32_t value) {
            seed ^= value;
            seed *= 16777619u;
        };

        mix(static_cast<std::uint32_t>(homeId));
        mix(static_cast<std::uint32_t>(awayId));
        mix(static_cast<std::uint32_t>(date.getYear()));
        mix(static_cast<std::uint32_t>(date.getMonth()));
        mix(static_cast<std::uint32_t>(date.getDay()));
        mix(static_cast<std::uint32_t>(date.getDayOfWeek()));

        return seed;
    }


    MatchLineupSnapshot buildSnapshotFromTeamSheet(const TeamSheet& teamSheet) {
        MatchLineupSnapshot snapshot;
        snapshot.teamId = teamSheet.teamId;
        snapshot.coachId = teamSheet.coachId;
        snapshot.formation = teamSheet.formation;
        snapshot.startingPlayerIds = teamSheet.startingPlayerIds;
        return snapshot;
    }

    void validateMatchLineupSnapshot(const MatchLineupSnapshot& snapshot) {
        if (snapshot.teamId == 0) {
            throw std::invalid_argument("lineup snapshot team id cannot be zero");
        }
        if (snapshot.coachId == 0) {
            throw std::invalid_argument("lineup snapshot coach id cannot be zero");
        }
        if (!isFormationSupported(snapshot.formation)) {
            throw std::invalid_argument("lineup snapshot formation is not supported");
        }
        if (snapshot.startingPlayerIds.size() > 11) {
            throw std::invalid_argument("lineup snapshot starter count cannot exceed 11");
        }

        std::unordered_set<PlayerId> seenStarterIds;
        seenStarterIds.reserve(snapshot.startingPlayerIds.size());
        for (PlayerId starterId : snapshot.startingPlayerIds) {
            if (starterId == 0) {
                throw std::invalid_argument("lineup snapshot starter id cannot be zero");
            }
            const auto [it, inserted] = seenStarterIds.emplace(starterId);
            (void)it;
            if (!inserted) {
                throw std::invalid_argument("lineup snapshot contains duplicate starter id");
            }
        }
    }

    int calculateStarterLineupStrength(const std::vector<const Footballer*>& starters) {
        if (starters.empty()) {
            throw std::logic_error("cannot calculate lineup strength from empty starter pool");
        }

        int totalPower = 0;
        for (const Footballer* starter : starters) {
            if (!starter) {
                throw std::logic_error("starter lineup contains null player pointer");
            }
            totalPower += starter->totalPower();
        }

        return totalPower / static_cast<int>(starters.size());
    }

    std::vector<const Footballer*> resolveStartersFromTeamSheet(const Team& team, const TeamSheet& teamSheet) {
        validateTeamSheetForTeam(teamSheet, team);

        std::vector<const Footballer*> starters;
        starters.reserve(teamSheet.startingPlayerIds.size());

        std::unordered_set<PlayerId> seenStarterIds;
        seenStarterIds.reserve(teamSheet.startingPlayerIds.size());

        for (PlayerId starterId : teamSheet.startingPlayerIds) {
            const auto [it, inserted] = seenStarterIds.emplace(starterId);
            (void)it;
            if (!inserted) {
                throw std::logic_error("teamsheet contains duplicate starter id");
            }

            const Footballer* player = team.findPlayerById(starterId);
            if (!player) {
                throw std::logic_error("teamsheet starter id could not be resolved in team");
            }

            starters.push_back(player);
        }

        return starters;
    }
}

const Footballer* pickFrom(const std::vector<const Footballer*>& players, std::mt19937& rng) {
    if (players.empty()) {
        return nullptr;
    }
    std::uniform_int_distribution<std::size_t> dist(0, players.size() - 1);
    return players[dist(rng)];
}

void ensurePlayerReport(MatchReport& report, TeamId teamId, const Footballer& player, bool started, int minutesPlayed) {
    const auto it = std::find_if(report.playerReports.begin(), report.playerReports.end(), [&player](const MatchPlayerReport& existing) {
        return existing.playerId == player.getId();
        });
    if (it != report.playerReports.end()) {
        return;
    }

    report.playerReports.push_back(MatchPlayerReport{
        player.getId(),
        teamId,
        started,
        minutesPlayed,
        0,
        0,
        0,
        0
        });
}

MatchPlayerReport* findPlayerReport(MatchReport& report, PlayerId playerId) {
    auto it = std::find_if(report.playerReports.begin(), report.playerReports.end(), [playerId](const MatchPlayerReport& entry) {
        return entry.playerId == playerId;
        });
    if (it == report.playerReports.end()) {
        return nullptr;
    }
    return &(*it);
}

int randomMinute(std::mt19937& rng) {
    std::uniform_int_distribution<int> minuteDist(1, 90);
    return minuteDist(rng);
}

MatchReport MatchSimulation::buildStrengthBasedReport(
    MatchId matchId,
    LeagueId leagueId,
    int seasonYear,
    int matchweek,
    const Team& homeTeam,
    const Team& awayTeam,
    const TeamSheet& homeSheet,
    const TeamSheet& awaySheet,
    const Date& date) {

    if (matchId == 0) {
        throw std::invalid_argument("matchId cannot be zero");
    }
    if (leagueId == 0) {
        throw std::invalid_argument("leagueId cannot be zero");
    }
    if (seasonYear < 0) {
        throw std::invalid_argument("seasonYear cannot be negative");
    }

    const std::vector<const Footballer*> homeStarters = resolveStartersFromTeamSheet(homeTeam, homeSheet);
    const std::vector<const Footballer*> awayStarters = resolveStartersFromTeamSheet(awayTeam, awaySheet);

    const int homeRating = calculateStarterLineupStrength(homeStarters);
    const int awayRating = calculateStarterLineupStrength(awayStarters);
    const int ratingDiff = homeRating - awayRating;

    const double homeAdvantage = 0.25;

    double homeExpectedGoals = 1.20 + homeAdvantage + (ratingDiff / 40.0);
    double awayExpectedGoals = 1.00 - (ratingDiff / 45.0);

    homeExpectedGoals = std::clamp(homeExpectedGoals, 0.2, 3.5);
    awayExpectedGoals = std::clamp(awayExpectedGoals, 0.2, 3.0);

    std::mt19937 rng(makeMatchSeed(homeTeam.getId(), awayTeam.getId(), date));

    std::poisson_distribution<int> homeDist(homeExpectedGoals);
    std::poisson_distribution<int> awayDist(awayExpectedGoals);

    MatchReport report;
    report.matchId = matchId;
    report.leagueId = leagueId;
    report.seasonYear = seasonYear;
    report.date = date;
    report.homeId = homeTeam.getId();
    report.awayId = awayTeam.getId();
    report.matchweek = matchweek;
    report.homeGoals = std::clamp(homeDist(rng), 0, 6);
    report.awayGoals = std::clamp(awayDist(rng), 0, 6);

    report.homeLineup = buildSnapshotFromTeamSheet(homeSheet);
    report.awayLineup = buildSnapshotFromTeamSheet(awaySheet);
    validateMatchLineupSnapshot(report.homeLineup);
    validateMatchLineupSnapshot(report.awayLineup);

    for (const Footballer* p : homeStarters) {
        ensurePlayerReport(report, homeTeam.getId(), *p, true, 90);
    }
    for (const Footballer* p : awayStarters) {
        ensurePlayerReport(report, awayTeam.getId(), *p, true, 90);
    }

    auto assignGoals = [&](int goals, TeamId teamId, const std::vector<const Footballer*>& starters) {
        for (int i = 0; i < goals; ++i) {
            const Footballer* scorer = pickFrom(starters, rng);
            if (!scorer) {
                continue;
            }

            ensurePlayerReport(report, teamId, *scorer, true, 90);

            const Footballer* assister = nullptr;
            if (starters.size() > 1) {
                std::bernoulli_distribution assistChance(0.75);
                if (assistChance(rng)) {
                    for (int attempt = 0; attempt < 4; ++attempt) {
                        const Footballer* candidate = pickFrom(starters, rng);
                        if (candidate && candidate->getId() != scorer->getId()) {
                            assister = candidate;
                            break;
                        }
                    }
                }
            }

            MatchPlayerReport* scorerReport = findPlayerReport(report, scorer->getId());
            if (scorerReport) {
                ++scorerReport->goals;
            }

            PlayerId assisterId = 0;
            if (assister) {
                ensurePlayerReport(report, teamId, *assister, true, 90);
                if (MatchPlayerReport* assisterReport = findPlayerReport(report, assister->getId())) {
                    ++assisterReport->assists;
                }
                assisterId = assister->getId();
            }

            report.events.push_back(MatchEventRecord{
                randomMinute(rng),
                MatchEventKind::Goal,
                teamId,
                scorer->getId(),
                assisterId
                });
        }
        };

    assignGoals(report.homeGoals, homeTeam.getId(), homeStarters);
    assignGoals(report.awayGoals, awayTeam.getId(), awayStarters);

    std::poisson_distribution<int> homeYellowDist(1.4);
    std::poisson_distribution<int> awayYellowDist(1.3);
    const int homeYellows = std::clamp(homeYellowDist(rng), 0, 4);
    const int awayYellows = std::clamp(awayYellowDist(rng), 0, 4);

    auto assignCards = [&](int cardCount, TeamId teamId, const std::vector<const Footballer*>& starters, MatchEventKind kind) {
        for (int i = 0; i < cardCount; ++i) {
            const Footballer* player = pickFrom(starters, rng);
            if (!player) {
                continue;
            }

            ensurePlayerReport(report, teamId, *player, true, 90);
            if (MatchPlayerReport* playerReport = findPlayerReport(report, player->getId())) {
                if (kind == MatchEventKind::YellowCard) {
                    ++playerReport->yellowCards;
                }
                else if (kind == MatchEventKind::RedCard) {
                    ++playerReport->redCards;
                }
            }

            report.events.push_back(MatchEventRecord{
                randomMinute(rng),
                kind,
                teamId,
                player->getId(),
                0
                });
        }
        };

    assignCards(homeYellows, homeTeam.getId(), homeStarters, MatchEventKind::YellowCard);
    assignCards(awayYellows, awayTeam.getId(), awayStarters, MatchEventKind::YellowCard);

    std::bernoulli_distribution redChance(0.06);
    if (redChance(rng)) {
        assignCards(1, homeTeam.getId(), homeStarters, MatchEventKind::RedCard);
    }
    if (redChance(rng)) {
        assignCards(1, awayTeam.getId(), awayStarters, MatchEventKind::RedCard);
    }

    return report;
}
