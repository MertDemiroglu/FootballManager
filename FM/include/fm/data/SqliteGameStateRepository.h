#pragma once

#include "fm/common/Date.h"
#include "fm/common/Types.h"
#include "fm/data/SavePolicy.h"
#include "fm/data/SqliteDatabase.h"
#include "fm/match/MatchReport.h"
#include "fm/match/TeamSheet.h"
#include "fm/transfer/TransferOffer.h"

#include <optional>
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

struct PersistedTeamSheetState {
    LeagueId leagueId = 0;
    TeamSheet teamSheet;
};

struct PersistedTeamFinanceState {
    LeagueId leagueId = 0;
    TeamId teamId = 0;
    Money totalBudget = 0;
    Money transferBudget = 0;
    Money wageBudget = 0;
};

struct PersistedPlayerRosterState {
    PlayerId playerId = 0;
    LeagueId leagueId = 0;
    TeamId teamId = 0;
    std::optional<Money> wage = std::nullopt;
    std::optional<int> contractYears = std::nullopt;
    int currentSeasonYear = -1;
};

struct PersistedTransferOfferState {
    OfferId offerId = 0;
    Date createdAt{ 1900, Month::January, 1 };
    Date lastValidDate{ 1900, Month::January, 1 };
    TransferOfferExpiryPolicy expiryPolicy = TransferOfferExpiryPolicy::FourteenDayLimit;
    LeagueId sellerLeagueId = 0;
    TeamId sellerTeamId = 0;
    LeagueId buyerLeagueId = 0;
    TeamId buyerTeamId = 0;
    PlayerId playerId = 0;
    Money fee = 0;
    TransferOfferStatus status = TransferOfferStatus::Pending;
    std::optional<TransferOfferResolution> resolution = std::nullopt;
};

struct PersistedSaveSettings {
    AutoSaveFrequency autoSaveFrequency = AutoSaveFrequency::Weekly;
    std::optional<Date> lastAutoSaveDate = std::nullopt;
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
    std::vector<PersistedTeamSheetState> loadTeamSheetStates() const;
    std::vector<PersistedPlayerRuntimeState> loadPlayerRuntimeStates() const;
    std::vector<PersistedTeamFinanceState> loadTeamFinanceStates() const;
    std::vector<PersistedPlayerRosterState> loadPlayerRosterStates() const;
    std::vector<PersistedTransferOfferState> loadTransferOfferStates() const;
    PersistedSaveSettings loadSaveSettings() const;
    void saveSaveSettings(const PersistedSaveSettings& settings) const;

    void saveRuntimeState(
        const Date& currentDate,
        int currentState,
        const std::vector<PersistedLeagueRuntimeState>& leagueStates,
        const std::vector<PersistedFixtureState>& fixtures,
        const std::vector<MatchReport>& reports,
        const std::vector<PersistedTeamSheetState>& teamSheetStates,
        const std::vector<PersistedPlayerRuntimeState>& playerStates,
        const std::vector<PersistedTeamFinanceState>& teamFinances,
        const std::vector<PersistedPlayerRosterState>& playerRosterStates,
        const std::vector<PersistedTransferOfferState>& transferOffers) const;
};
