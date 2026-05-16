#include "fm/data/RuntimeSaveValidator.h"

#include "fm/data/BootstrapDtos.h"
#include "fm/data/SqliteBootstrapRepository.h"
#include "fm/data/SqliteGameStateRepository.h"
#include "fm/data/SqliteLeagueRulesRepository.h"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace {
    std::string dateToIsoString(const Date& date) {
        std::ostringstream output;
        output << std::setw(4) << std::setfill('0') << date.getYear() << "-"
            << std::setw(2) << std::setfill('0') << static_cast<int>(date.getMonth()) << "-"
            << std::setw(2) << std::setfill('0') << date.getDay();
        return output.str();
    }

    bool sameDate(const Date& lhs, const Date& rhs) {
        return lhs.getYear() == rhs.getYear()
            && lhs.getMonth() == rhs.getMonth()
            && lhs.getDay() == rhs.getDay();
    }

    SaveValidationResult invalid(std::string reason) {
        SaveValidationResult result;
        result.valid = false;
        result.reason = std::move(reason);
        return result;
    }

    SaveValidationResult valid(Date currentDate) {
        SaveValidationResult result;
        result.valid = true;
        result.currentDate = currentDate;
        return result;
    }

    bool dateIsBefore(const Date& lhs, const Date& rhs) {
        return lhs < rhs;
    }

    bool dateIsOnOrAfter(const Date& lhs, const Date& rhs) {
        return !dateIsBefore(lhs, rhs);
    }

    std::int64_t compoundTeamKey(LeagueId leagueId, TeamId teamId) {
        return (static_cast<std::int64_t>(leagueId) << 32)
            ^ static_cast<std::int64_t>(teamId);
    }

    struct RuntimeRosterLocation {
        LeagueId leagueId = 0;
        TeamId teamId = 0;
    };

    bool sameTeamKey(const RuntimeRosterLocation& location, LeagueId leagueId, TeamId teamId) {
        return location.leagueId == leagueId && location.teamId == teamId;
    }

    std::string formatTeamKey(LeagueId leagueId, TeamId teamId) {
        return "league_id=" + std::to_string(leagueId) + ", team_id=" + std::to_string(teamId);
    }

    std::string teamSheetOwnershipError(
        const char* role,
        LeagueId sheetLeagueId,
        TeamId sheetTeamId,
        PlayerId playerId,
        const RuntimeRosterLocation* actualLocation) {
        std::string message = "runtime team sheet references a ";
        message += role;
        message += " outside its team roster (player_id=";
        message += std::to_string(playerId);
        message += ", expected ";
        message += formatTeamKey(sheetLeagueId, sheetTeamId);
        message += ", actual ";
        if (actualLocation) {
            message += formatTeamKey(actualLocation->leagueId, actualLocation->teamId);
        } else {
            message += "missing from runtime_player_roster_state";
        }
        message += ")";
        return message;
    }

    SaveValidationResult validateRuntimeState(
        const Date& currentDate,
        const std::vector<PersistedLeagueRuntimeState>& leagueStates,
        const std::vector<PersistedFixtureState>& fixtures,
        const std::vector<PersistedTeamSheetState>& teamSheetStates,
        const std::vector<PersistedPlayerRuntimeState>& playerStates,
        const std::vector<PersistedTeamFinanceState>& teamFinanceStates,
        const std::vector<PersistedPlayerRosterState>& playerRosterStates,
        const std::vector<PersistedTransferOfferState>& transferOfferStates,
        const std::vector<LeagueSeedData>& seedLeagues,
        const std::unordered_map<LeagueId, LeagueRules>& rulesByLeague,
        const SaveMetadata* metadata) {
        if (leagueStates.empty()) {
            return invalid("save slot is missing league runtime state");
        }
        if (playerStates.empty()) {
            return invalid("save slot is missing player runtime state");
        }
        if (teamFinanceStates.empty()) {
            return invalid("save slot is missing runtime team finance state");
        }
        if (playerRosterStates.empty()) {
            return invalid("save slot is missing runtime player roster state");
        }

        if (metadata && metadata->currentDate != dateToIsoString(currentDate)) {
            return invalid("save metadata current date does not mirror game_state current date");
        }

        std::unordered_map<LeagueId, PersistedLeagueRuntimeState> stateByLeague;
        for (const PersistedLeagueRuntimeState& state : leagueStates) {
            if (state.leagueId == 0) {
                return invalid("league runtime state has an invalid league id");
            }
            if (state.seasonYear < 0) {
                return invalid("league runtime state has an invalid season year");
            }

            const auto rulesIt = rulesByLeague.find(state.leagueId);
            if (rulesIt == rulesByLeague.end()) {
                return invalid("league runtime state has no SQL league rules");
            }

            const LeagueRules& rules = rulesIt->second;
            const Date preseasonStart = rules.preseasonStart(state.seasonYear);
            const Date nextPreseasonStart = rules.nextPreseasonStart(state.seasonYear);
            if (dateIsBefore(currentDate, preseasonStart)
                || (dateIsOnOrAfter(currentDate, nextPreseasonStart) && !sameDate(currentDate, nextPreseasonStart))) {
                return invalid("game_state current date is outside the league runtime season window");
            }

            stateByLeague.emplace(state.leagueId, state);
        }

        std::unordered_set<std::int64_t> knownTeams;
        std::unordered_set<PlayerId> knownPlayers;
        for (const LeagueSeedData& league : seedLeagues) {
            for (const TeamSeedData& team : league.teams) {
                knownTeams.insert(compoundTeamKey(league.id, team.id));
                for (const PlayerSeedData& player : team.players) {
                    knownPlayers.insert(player.id);
                }
            }
        }

        std::unordered_set<std::int64_t> seenFinanceRows;
        for (const PersistedTeamFinanceState& finance : teamFinanceStates) {
            if (stateByLeague.find(finance.leagueId) == stateByLeague.end()) {
                return invalid("runtime team finance references a league without runtime state");
            }
            if (finance.teamId == 0) {
                return invalid("runtime team finance has an invalid team id");
            }
            if (knownTeams.find(compoundTeamKey(finance.leagueId, finance.teamId)) == knownTeams.end()) {
                return invalid("runtime team finance references unknown team");
            }
            if (!seenFinanceRows.insert(compoundTeamKey(finance.leagueId, finance.teamId)).second) {
                return invalid("runtime team finance has a duplicate league/team row");
            }
            if (finance.totalBudget < 0 || finance.transferBudget < 0 || finance.wageBudget < 0) {
                return invalid("runtime team finance has a negative budget");
            }
        }

        std::unordered_map<PlayerId, RuntimeRosterLocation> rosterTeamByPlayer;
        for (const PersistedPlayerRosterState& roster : playerRosterStates) {
            if (roster.playerId == 0) {
                return invalid("runtime player roster state has an invalid player id");
            }
            if (knownPlayers.find(roster.playerId) == knownPlayers.end()) {
                return invalid("runtime player roster state references unknown player");
            }
            if (stateByLeague.find(roster.leagueId) == stateByLeague.end()) {
                return invalid("runtime player roster state references a league without runtime state");
            }
            if (roster.teamId == 0) {
                return invalid("runtime player roster state has an invalid team id");
            }
            const std::int64_t rosterTeamKey = compoundTeamKey(roster.leagueId, roster.teamId);
            if (knownTeams.find(rosterTeamKey) == knownTeams.end()) {
                return invalid("runtime player roster state references unknown team");
            }
            if (roster.wage.has_value() != roster.contractYears.has_value()) {
                return invalid("runtime player roster state has partial contract snapshot");
            }
            if (roster.wage.has_value() && *roster.wage < 0) {
                return invalid("runtime player roster state has negative wage");
            }
            if (roster.contractYears.has_value() && *roster.contractYears <= 0) {
                return invalid("runtime player roster state has invalid contract years");
            }
            if (roster.currentSeasonYear < 0) {
                return invalid("runtime player roster state has invalid current season year");
            }
            if (!rosterTeamByPlayer.emplace(roster.playerId, RuntimeRosterLocation{ roster.leagueId, roster.teamId }).second) {
                return invalid("runtime player roster state has duplicate player id");
            }
        }

        for (PlayerId playerId : knownPlayers) {
            if (rosterTeamByPlayer.find(playerId) == rosterTeamByPlayer.end()) {
                return invalid("runtime player roster state is missing bootstrap player");
            }
        }

        for (const PersistedPlayerRuntimeState& playerState : playerStates) {
            if (rosterTeamByPlayer.find(playerState.playerId) == rosterTeamByPlayer.end()) {
                return invalid("player runtime state is missing matching player roster state");
            }
        }

        std::unordered_set<std::int64_t> seenTeamsWithSheets;
        for (const PersistedTeamSheetState& state : teamSheetStates) {
            if (stateByLeague.find(state.leagueId) == stateByLeague.end()) {
                return invalid("runtime team sheet references a league without runtime state");
            }
            if (state.teamSheet.teamId == 0) {
                return invalid("runtime team sheet has an invalid team id");
            }
            if (!isFormationSupported(state.teamSheet.formation)) {
                return invalid("runtime team sheet has an invalid formation");
            }

            const std::int64_t sheetTeamKey = compoundTeamKey(state.leagueId, state.teamSheet.teamId);
            if (knownTeams.find(sheetTeamKey) == knownTeams.end()) {
                return invalid("runtime team sheet references unknown team");
            }
            if (!seenTeamsWithSheets.insert(sheetTeamKey).second) {
                return invalid("runtime team sheet has a duplicate league/team row");
            }

            if (state.teamSheet.substitutePlayerIds.size() > kMaxSubstituteCount) {
                return invalid("runtime team sheet has too many substitutes");
            }

            const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(state.teamSheet.formation);
            if (state.teamSheet.startingAssignments.size() > slotTemplate.size()) {
                return invalid("runtime team sheet has too many starter assignments");
            }
            if (state.teamSheet.startingAssignments.size() != state.teamSheet.startingPlayerIds.size()) {
                return invalid("runtime team sheet starter ids do not match assignment count");
            }

            std::unordered_set<PlayerId> starterIds;
            for (std::size_t i = 0; i < state.teamSheet.startingAssignments.size(); ++i) {
                const TeamSheetSlotAssignment& assignment = state.teamSheet.startingAssignments[i];
                if (assignment.playerId == 0) {
                    return invalid("runtime team sheet starter has an invalid player id");
                }
                if (state.teamSheet.startingPlayerIds[i] != assignment.playerId) {
                    return invalid("runtime team sheet starter order does not match assignment ids");
                }
                if (assignment.slotIndex >= slotTemplate.size()
                    || slotTemplate[assignment.slotIndex] != assignment.slotRole) {
                    return invalid("runtime team sheet starter slot role does not match formation template");
                }
                if (!starterIds.insert(assignment.playerId).second) {
                    return invalid("runtime team sheet has duplicate starter player ids");
                }
                const auto rosterIt = rosterTeamByPlayer.find(assignment.playerId);
                if (rosterIt == rosterTeamByPlayer.end()
                    || !sameTeamKey(rosterIt->second, state.leagueId, state.teamSheet.teamId)) {
                    return invalid(teamSheetOwnershipError(
                        "starter",
                        state.leagueId,
                        state.teamSheet.teamId,
                        assignment.playerId,
                        rosterIt == rosterTeamByPlayer.end() ? nullptr : &rosterIt->second));
                }
            }

            std::unordered_set<PlayerId> substituteIds;
            for (PlayerId playerId : state.teamSheet.substitutePlayerIds) {
                if (playerId == 0) {
                    return invalid("runtime team sheet substitute has an invalid player id");
                }
                if (starterIds.find(playerId) != starterIds.end()) {
                    return invalid("runtime team sheet has starter/substitute overlap");
                }
                if (!substituteIds.insert(playerId).second) {
                    return invalid("runtime team sheet has duplicate substitute player ids");
                }
                const auto rosterIt = rosterTeamByPlayer.find(playerId);
                if (rosterIt == rosterTeamByPlayer.end()
                    || !sameTeamKey(rosterIt->second, state.leagueId, state.teamSheet.teamId)) {
                    return invalid(teamSheetOwnershipError(
                        "substitute",
                        state.leagueId,
                        state.teamSheet.teamId,
                        playerId,
                        rosterIt == rosterTeamByPlayer.end() ? nullptr : &rosterIt->second));
                }
            }
        }

        std::unordered_map<LeagueId, bool> sawFixtureByLeague;
        for (const PersistedFixtureState& fixture : fixtures) {
            const auto stateIt = stateByLeague.find(fixture.leagueId);
            if (stateIt == stateByLeague.end()) {
                return invalid("runtime fixture references a league without runtime state");
            }
            if (fixture.seasonYear != stateIt->second.seasonYear) {
                return invalid("runtime fixture season year does not match league runtime state");
            }
            if (fixture.matchId == 0) {
                return invalid("runtime fixture has an invalid match id");
            }
            if (fixture.matchweek <= 0) {
                return invalid("runtime fixture has an invalid matchweek");
            }
            if (fixture.homeTeamId == 0 || fixture.awayTeamId == 0) {
                return invalid("runtime fixture has an invalid team id");
            }
            if (fixture.date.getYear() < fixture.seasonYear || fixture.date.getYear() > fixture.seasonYear + 1) {
                return invalid("runtime fixture date is outside its season year");
            }
            const auto rulesIt = rulesByLeague.find(fixture.leagueId);
            if (rulesIt == rulesByLeague.end()) {
                return invalid("runtime fixture references a league without SQL league rules");
            }
            const Date kickoffDate = rulesIt->second.kickoffDate(fixture.seasonYear);
            const Date nextPreseasonStart = rulesIt->second.nextPreseasonStart(fixture.seasonYear);
            if (dateIsBefore(fixture.date, kickoffDate) || !dateIsBefore(fixture.date, nextPreseasonStart)) {
                return invalid("runtime fixture date is outside its league season fixture window");
            }
            sawFixtureByLeague[fixture.leagueId] = true;
        }

        for (const PersistedLeagueRuntimeState& state : leagueStates) {
            if (state.fixtureGenerated && !sawFixtureByLeague[state.leagueId]) {
                return invalid("league runtime state says fixtures are generated but no fixtures were persisted");
            }
        }

        std::unordered_set<OfferId> seenOfferIds;
        std::unordered_set<std::string> pendingOfferKeys;
        for (const PersistedTransferOfferState& offer : transferOfferStates) {
            if (offer.offerId == 0) {
                return invalid("runtime transfer offer has an invalid offer id");
            }
            if (!seenOfferIds.insert(offer.offerId).second) {
                return invalid("runtime transfer offer has a duplicate offer id");
            }
            if (offer.lastValidDate < offer.createdAt) {
                return invalid("runtime transfer offer created_at is after last_valid_date");
            }
            if (offer.fee <= 0) {
                return invalid("runtime transfer offer has an invalid fee");
            }
            if (stateByLeague.find(offer.sellerLeagueId) == stateByLeague.end()
                || rulesByLeague.find(offer.sellerLeagueId) == rulesByLeague.end()) {
                return invalid("runtime transfer offer seller league is not in runtime league state");
            }
            if (stateByLeague.find(offer.buyerLeagueId) == stateByLeague.end()
                || rulesByLeague.find(offer.buyerLeagueId) == rulesByLeague.end()) {
                return invalid("runtime transfer offer buyer league is not in runtime league state");
            }
            if (offer.sellerTeamId == 0 || offer.buyerTeamId == 0) {
                return invalid("runtime transfer offer has an invalid team id");
            }
            if (offer.playerId == 0) {
                return invalid("runtime transfer offer has an invalid player id");
            }
            if (offer.sellerLeagueId == offer.buyerLeagueId && offer.sellerTeamId == offer.buyerTeamId) {
                return invalid("runtime transfer offer seller and buyer are the same team");
            }
            if (offer.status == TransferOfferStatus::Pending) {
                if (offer.resolution.has_value()) {
                    return invalid("pending runtime transfer offer has a resolution");
                }
                if (offer.lastValidDate < currentDate) {
                    return invalid("pending runtime transfer offer is past last_valid_date");
                }

                std::ostringstream pendingKey;
                pendingKey << offer.sellerLeagueId << ":"
                    << offer.sellerTeamId << ":"
                    << offer.buyerLeagueId << ":"
                    << offer.buyerTeamId << ":"
                    << offer.playerId;
                if (!pendingOfferKeys.insert(pendingKey.str()).second) {
                    return invalid("runtime transfer offer has duplicate pending seller/buyer/player row");
                }
            }
            else if (offer.status == TransferOfferStatus::Resolved) {
                if (!offer.resolution.has_value()) {
                    return invalid("resolved runtime transfer offer is missing resolution");
                }
            }
            else {
                return invalid("runtime transfer offer has an unsupported status");
            }
            // TODO: Validate pending player ownership against the seller team once
            // runtime validation owns a canonical team/player lookup snapshot.
        }

        return valid(currentDate);
    }
}

SaveValidationResult RuntimeSaveValidator::validateExistingSave(
    const std::string& databasePath,
    const SaveMetadata* metadata) {
    try {
        SqliteGameStateRepository runtimeRepository(databasePath, GameStateRepositoryMode::ReadExisting);
        if (!runtimeRepository.hasGameState()) {
            return invalid("save slot is missing runtime game_state");
        }

        const Date currentDate = runtimeRepository.loadCurrentDate();
        const std::vector<PersistedLeagueRuntimeState> leagueStates = runtimeRepository.loadLeagueRuntimeStates();
        const std::vector<PersistedFixtureState> fixtures = runtimeRepository.loadFixtures();
        const std::vector<PersistedTeamSheetState> teamSheetStates = runtimeRepository.loadTeamSheetStates();
        const std::vector<PersistedPlayerRuntimeState> playerStates = runtimeRepository.loadPlayerRuntimeStates();
        const std::vector<PersistedTeamFinanceState> teamFinanceStates = runtimeRepository.loadTeamFinanceStates();
        const std::vector<PersistedPlayerRosterState> playerRosterStates = runtimeRepository.loadPlayerRosterStates();
        const std::vector<PersistedTransferOfferState> transferOfferStates = runtimeRepository.loadTransferOfferStates();
        SqliteBootstrapRepository bootstrapRepository(databasePath);
        const std::vector<LeagueSeedData> seedLeagues = bootstrapRepository.loadLeagues();
        SqliteLeagueRulesRepository rulesRepository(databasePath);
        const std::unordered_map<LeagueId, LeagueRules> rulesByLeague = rulesRepository.loadAllLeagueRules();
        return validateRuntimeState(
            currentDate,
            leagueStates,
            fixtures,
            teamSheetStates,
            playerStates,
            teamFinanceStates,
            playerRosterStates,
            transferOfferStates,
            seedLeagues,
            rulesByLeague,
            metadata);
    } catch (const std::exception& ex) {
        return invalid(ex.what());
    } catch (...) {
        return invalid("unknown runtime save validation error");
    }
}

SaveValidationResult RuntimeSaveValidator::validateNewSave(
    const std::string& databasePath,
    const SaveMetadata& metadata,
    const Date& expectedInitialDate) {
    SaveValidationResult result = validateExistingSave(databasePath, &metadata);
    if (!result.valid) {
        return result;
    }

    if (!sameDate(result.currentDate, expectedInitialDate)) {
        return invalid(
            "new save persisted a game_state date different from the initial game date: expected "
            + dateToIsoString(expectedInitialDate)
            + " but got "
            + dateToIsoString(result.currentDate));
    }

    try {
        SqliteGameStateRepository runtimeRepository(databasePath, GameStateRepositoryMode::ReadExisting);
        SqliteLeagueRulesRepository rulesRepository(databasePath);
        const std::unordered_map<LeagueId, LeagueRules> rulesByLeague = rulesRepository.loadAllLeagueRules();
        const std::vector<PersistedLeagueRuntimeState> leagueStates = runtimeRepository.loadLeagueRuntimeStates();
        const std::vector<PersistedFixtureState> fixtures = runtimeRepository.loadFixtures();

        for (const PersistedLeagueRuntimeState& state : leagueStates) {
            if (state.seasonYear != expectedInitialDate.getYear()) {
                return invalid("new save persisted a league season year different from the initial game date");
            }
        }

        bool sawInitialSeasonFixture = false;
        for (const PersistedFixtureState& fixture : fixtures) {
            const auto rulesIt = rulesByLeague.find(fixture.leagueId);
            if (rulesIt == rulesByLeague.end()) {
                return invalid("new save fixture references a league without SQL league rules");
            }
            const Date kickoffDate = rulesIt->second.kickoffDate(fixture.seasonYear);
            const Date nextPreseasonStart = rulesIt->second.nextPreseasonStart(fixture.seasonYear);
            if (fixture.seasonYear == expectedInitialDate.getYear()
                && !dateIsBefore(fixture.date, kickoffDate)
                && dateIsBefore(fixture.date, nextPreseasonStart)) {
                sawInitialSeasonFixture = true;
                break;
            }
        }
        if (!sawInitialSeasonFixture) {
            return invalid("new save did not persist an initial-season fixture inside the SQL league kickoff window");
        }
    } catch (const std::exception& ex) {
        return invalid(ex.what());
    } catch (...) {
        return invalid("unknown new save validation error");
    }

    return result;
}
