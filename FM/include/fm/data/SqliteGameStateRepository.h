#pragma once

#include "fm/common/Date.h"
#include "fm/common/Types.h"
#include "fm/data/SqliteDatabase.h"
#include "fm/match/MatchReport.h"

#include <string>
#include <vector>

enum class GameStateRepositoryMode {
    EnsureSchema,
    ReadExisting
};

struct PersistedLeagueRuntimeState {
    LeagueId leagueId = 0;
    int seasonYear = -1;
    bool fixtureGenerated = false;
    int lastSeasonRolloverYear = -1;
};

struct PersistedFixtureState {
    MatchId matchId = 0;
    LeagueId leagueId = 0;
    int seasonYear = -1;
    int matchweek = 0;
    Date date{ 1900, Month::January, 1 };
    TeamId homeTeamId = 0;
    TeamId awayTeamId = 0;
    bool played = false;
    bool eventEnqueued = false;
    int homeGoals = -1;
    int awayGoals = -1;
};

struct PersistedPlayerRuntimeState {
    PlayerId playerId = 0;
    int form = 50;
    int fitness = 100;
    int morale = 50;
};

class SqliteGameStateRepository {
private:
    SqliteDatabase database;

public:
    explicit SqliteGameStateRepository(
        const std::string& databasePath,
        GameStateRepositoryMode mode = GameStateRepositoryMode::EnsureSchema);

    bool hasGameState() const;
    Date loadCurrentDate() const;
    std::vector<PersistedLeagueRuntimeState> loadLeagueRuntimeStates() const;
    std::vector<PersistedFixtureState> loadFixtures() const;
    std::vector<MatchReport> loadMatchReports() const;
    std::vector<PersistedPlayerRuntimeState> loadPlayerRuntimeStates() const;

    void updateCurrentDate(const Date& currentDate) const;

    void saveRuntimeState(
        const Date& currentDate,
        int currentState,
        const std::vector<PersistedLeagueRuntimeState>& leagueStates,
        const std::vector<PersistedFixtureState>& fixtures,
        const std::vector<MatchReport>& reports,
        const std::vector<PersistedPlayerRuntimeState>& playerStates) const;
};
