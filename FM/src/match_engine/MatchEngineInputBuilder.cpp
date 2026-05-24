#include"fm/match_engine/MatchEngineInputBuilder.h"

#include"fm/match/TeamSheet.h"
#include"fm/roster/Footballer.h"
#include"fm/roster/Team.h"

#include<cstdint>
#include<stdexcept>
#include<string>
#include<unordered_set>
#include<utility>

namespace {
    constexpr std::uint64_t FnvOffsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t FnvPrime = 1099511628211ull;

    void mixByte(std::uint64_t& hash, std::uint8_t value) {
        hash ^= value;
        hash *= FnvPrime;
    }

    void mixUint64(std::uint64_t& hash, std::uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            mixByte(hash, static_cast<std::uint8_t>((value >> (i * 8)) & 0xffu));
        }
    }

    std::runtime_error buildValidationError(const std::string& message) {
        return std::runtime_error("MatchEngineInputBuilder: " + message);
    }

    void validateNonZeroIds(
        MatchId matchId,
        LeagueId leagueId,
        TeamId homeTeamId,
        TeamId awayTeamId) {
        if (matchId == 0) {
            throw buildValidationError("match id cannot be zero");
        }
        if (leagueId == 0) {
            throw buildValidationError("league id cannot be zero");
        }
        if (homeTeamId == 0) {
            throw buildValidationError("home team id cannot be zero");
        }
        if (awayTeamId == 0) {
            throw buildValidationError("away team id cannot be zero");
        }
        if (homeTeamId == awayTeamId) {
            throw buildValidationError("home and away team ids must be different");
        }
    }

    void validateMatchMetadata(int seasonYear, int matchweek) {
        if (seasonYear < 0) {
            throw buildValidationError("season year cannot be negative");
        }
        if (matchweek < 0) {
            throw buildValidationError("matchweek cannot be negative");
        }
    }

    MatchPlayerSnapshot buildPlayerSnapshot(const Footballer& player, TeamId expectedTeamId) {
        if (player.getId() == 0) {
            throw buildValidationError("team roster contains player id zero");
        }
        if (player.getTeamId() != expectedTeamId) {
            throw buildValidationError("team roster contains player with mismatched team id");
        }

        const int totalPower = player.totalPower();

        MatchPlayerSnapshot snapshot;
        snapshot.playerId = player.getId();
        snapshot.teamId = player.getTeamId();
        snapshot.position = player.getPlayerPosition();
        snapshot.attributes = player.getAttributes();
        snapshot.conditionState = player.getConditionState();
        snapshot.baseOverall = totalPower;
        snapshot.totalPower = totalPower;
        return snapshot;
    }

    MatchTeamSnapshot buildTeamSnapshot(
        LeagueId leagueId,
        const Team& team,
        const TeamSheet& teamSheet) {
        try {
            validateTeamSheetForTeam(teamSheet, team);
        }
        catch (const std::exception& ex) {
            throw buildValidationError(std::string("invalid teamsheet for team ")
                + std::to_string(team.getId())
                + ": "
                + ex.what());
        }

        MatchTeamSnapshot snapshot;
        snapshot.leagueId = leagueId;
        snapshot.teamId = team.getId();
        snapshot.teamName = team.getName();
        snapshot.tacticalSetup = teamSheet.tacticalSetup;
        snapshot.teamSheet = teamSheet;
        snapshot.players.reserve(team.getPlayers().size());

        std::unordered_set<PlayerId> playerIds;
        playerIds.reserve(team.getPlayers().size());

        for (const auto& player : team.getPlayers()) {
            if (!player) {
                throw buildValidationError("team roster contains null player");
            }

            MatchPlayerSnapshot playerSnapshot = buildPlayerSnapshot(*player, team.getId());
            const auto [it, inserted] = playerIds.insert(playerSnapshot.playerId);
            (void)it;
            if (!inserted) {
                throw buildValidationError("team roster contains duplicate player id");
            }

            snapshot.players.push_back(std::move(playerSnapshot));
        }

        return snapshot;
    }

    void validateDisjointSnapshots(
        const MatchTeamSnapshot& homeTeam,
        const MatchTeamSnapshot& awayTeam) {
        std::unordered_set<PlayerId> homePlayerIds;
        homePlayerIds.reserve(homeTeam.players.size());

        for (const MatchPlayerSnapshot& player : homeTeam.players) {
            homePlayerIds.insert(player.playerId);
        }

        for (const MatchPlayerSnapshot& player : awayTeam.players) {
            if (homePlayerIds.find(player.playerId) != homePlayerIds.end()) {
                throw buildValidationError("player appears in both home and away snapshots");
            }
        }
    }
}

std::uint64_t buildDeterministicMatchSeed(
    MatchId matchId,
    LeagueId leagueId,
    const Date& matchDate,
    TeamId homeTeamId,
    TeamId awayTeamId) {
    std::uint64_t hash = FnvOffsetBasis;

    mixUint64(hash, matchId);
    mixUint64(hash, leagueId);
    mixUint64(hash, static_cast<std::uint64_t>(matchDate.getYear()));
    mixUint64(hash, static_cast<std::uint64_t>(static_cast<int>(matchDate.getMonth())));
    mixUint64(hash, static_cast<std::uint64_t>(matchDate.getDay()));
    mixUint64(hash, homeTeamId);
    mixUint64(hash, awayTeamId);

    return hash == 0 ? FnvOffsetBasis : hash;
}

MatchEngineInput MatchEngineInputBuilder::build(
    MatchId matchId,
    LeagueId leagueId,
    int seasonYear,
    int matchweek,
    const Date& matchDate,
    const Team& homeTeam,
    const Team& awayTeam,
    const TeamSheet& homeSheet,
    const TeamSheet& awaySheet,
    MatchEngineOptions options) const {
    validateNonZeroIds(matchId, leagueId, homeTeam.getId(), awayTeam.getId());
    validateMatchMetadata(seasonYear, matchweek);

    if (options.deterministicSeed == 0) {
        options.deterministicSeed = buildDeterministicMatchSeed(
            matchId,
            leagueId,
            matchDate,
            homeTeam.getId(),
            awayTeam.getId());
    }

    MatchEngineInput input;
    input.matchId = matchId;
    input.leagueId = leagueId;
    input.seasonYear = seasonYear;
    input.matchweek = matchweek;
    input.matchDate = matchDate;
    input.homeTeam = buildTeamSnapshot(leagueId, homeTeam, homeSheet);
    input.awayTeam = buildTeamSnapshot(leagueId, awayTeam, awaySheet);
    input.options = options;

    validateDisjointSnapshots(input.homeTeam, input.awayTeam);

    return input;
}

MatchEngineInput MatchEngineInputBuilder::build(
    MatchId matchId,
    LeagueId leagueId,
    const Date& matchDate,
    const Team& homeTeam,
    const Team& awayTeam,
    const TeamSheet& homeSheet,
    const TeamSheet& awaySheet,
    MatchEngineOptions options) const {
    return build(
        matchId,
        leagueId,
        matchDate.getYear(),
        0,
        matchDate,
        homeTeam,
        awayTeam,
        homeSheet,
        awaySheet,
        options);
}
