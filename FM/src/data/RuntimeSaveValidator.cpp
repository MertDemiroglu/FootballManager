#include "fm/data/RuntimeSaveValidator.h"

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

    SaveValidationResult validateRuntimeState(
        const Date& currentDate,
        const std::vector<PersistedLeagueRuntimeState>& leagueStates,
        const std::vector<PersistedFixtureState>& fixtures,
        const std::vector<PersistedTeamSheetState>& teamSheetStates,
        const std::vector<PersistedPlayerRuntimeState>& playerStates,
        const std::unordered_map<LeagueId, LeagueRules>& rulesByLeague,
        const SaveMetadata* metadata) {
        if (leagueStates.empty()) {
            return invalid("save slot is missing league runtime state");
        }
        if (playerStates.empty()) {
            return invalid("save slot is missing player runtime state");
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

            const std::int64_t compoundTeamKey = (static_cast<std::int64_t>(state.leagueId) << 32)
                ^ static_cast<std::int64_t>(state.teamSheet.teamId);
            if (!seenTeamsWithSheets.insert(compoundTeamKey).second) {
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
        SqliteLeagueRulesRepository rulesRepository(databasePath);
        const std::unordered_map<LeagueId, LeagueRules> rulesByLeague = rulesRepository.loadAllLeagueRules();
        return validateRuntimeState(currentDate, leagueStates, fixtures, teamSheetStates, playerStates, rulesByLeague, metadata);
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
