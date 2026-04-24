#pragma once

#include<QObject>
#include<QString>
#include<QVariantList>
#include<QVariantMap>

#include"StandingsTableModel.h"
#include"TeamPlayersModel.h"
#include"TeamRecentMatchesModel.h"
#include"TeamUpcomingMatchesModel.h"
#include"PendingTransferOffersModel.h"
#include"TeamSeasonStatsObject.h"
#include"DashboardStateObject.h"
#include"DashboardUpcomingMatchesModel.h"
#include"InteractionStateObject.h"
#include"ShellStateObject.h"
#include"EditableLineupStateObject.h"
#include"EditableLineupSlotsModel.h"
#include"EditableLineupRosterModel.h"

#include<memory>
#include<optional>

#include"fm/common/Types.h"
#include"fm/match/EditableLineup.h"

class Game;
class Footballer;
class PreMatchInteraction;
class PostMatchInteraction;
class TransferOfferDecisionInteraction;
class League;
class LeagueContext;


struct Date;
struct FixtureMatchPreview;
struct StandingsEntry;
struct TeamSeasonStats;
struct MatchRecord;
struct MatchReport;
struct PlayerSeasonStats;
struct TransferOffer;
struct TeamSheet;
struct MatchLineupSnapshot;
enum class FormationId;
enum class FormationSlotRole;

enum class TransferOfferExpiryPolicy;
enum class GameState;

class GameFacade : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(GameFacade)
    Q_PROPERTY(QString lastError READ getLastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QAbstractListModel* standingsModel READ getStandingsModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* currentTeamPlayersModel READ getCurrentTeamPlayersModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* currentTeamRecentMatchesModel READ getCurrentTeamRecentMatchesModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* currentTeamUpcomingMatchesModel READ getCurrentTeamUpcomingMatchesModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* pendingTransferOffersModel READ getPendingTransferOffersModel CONSTANT)
    Q_PROPERTY(TeamSeasonStatsObject* currentTeamSeasonStatsObject READ getCurrentTeamSeasonStatsObject CONSTANT)
    Q_PROPERTY(DashboardStateObject* dashboardState READ getDashboardState CONSTANT)
    Q_PROPERTY(QAbstractListModel* dashboardUpcomingMatchesModel READ getDashboardUpcomingMatchesModel CONSTANT)
    Q_PROPERTY(ShellStateObject* shellState READ getShellState CONSTANT)
    Q_PROPERTY(InteractionStateObject* interactionState READ getInteractionState CONSTANT)
    Q_PROPERTY(EditableLineupStateObject* editableLineupState READ getEditableLineupState CONSTANT)
    Q_PROPERTY(EditableLineupSlotsModel* editableLineupSlotsModel READ getEditableLineupSlotsModel CONSTANT)
    Q_PROPERTY(EditableLineupRosterModel* editableLineupRosterModel READ getEditableLineupRosterModel CONSTANT)

public:
    explicit GameFacade(QObject* parent = nullptr);
    ~GameFacade();

    Q_INVOKABLE QVariantList getTeamSelectionList() const;
    Q_INVOKABLE bool startNewGame(int teamId, const QString& managerName);
    Q_INVOKABLE bool startNewGameForLeagueTeam(int leagueId, int teamId, const QString& managerName);
    Q_INVOKABLE bool hasStartedGame() const;

    Q_INVOKABLE QString getCurrentDateText() const;
    Q_INVOKABLE QString getCurrentStateText() const;
    Q_INVOKABLE QString getSelectedTeamName() const;
    Q_INVOKABLE int getSelectedLeagueId() const;
    Q_INVOKABLE int getSelectedTeamId() const;
    Q_INVOKABLE QString getManagerName() const;
    Q_INVOKABLE QString getLastError() const;
    Q_INVOKABLE void clearLastError();

    Q_INVOKABLE QVariantMap getDashboard() const;

    Q_INVOKABLE bool advanceOneDay();
    Q_INVOKABLE bool advanceDays(int count);
    Q_INVOKABLE bool isTimePaused() const;
    Q_INVOKABLE bool pauseSimulation();
    Q_INVOKABLE bool resumeSimulation();

    Q_INVOKABLE bool hasActiveInteraction() const;
    Q_INVOKABLE QString getActiveInteractionKind() const;

    Q_INVOKABLE QVariantMap getActivePreMatchInteraction() const;
    Q_INVOKABLE QVariantMap getActivePostMatchInteraction() const;
    Q_INVOKABLE QVariantMap getActiveTransferOfferInteraction() const;
    Q_INVOKABLE bool playActiveMatch();
    Q_INVOKABLE bool resolveActiveInteraction();

    Q_INVOKABLE bool acceptActiveTransferOffer();
    Q_INVOKABLE bool rejectActiveTransferOffer();
    Q_INVOKABLE bool deferActiveTransferOffer();
    Q_INVOKABLE QVariantList getPendingTransferOffers() const;
    Q_INVOKABLE bool acceptTransferOfferById(int offerId);
    Q_INVOKABLE bool rejectTransferOfferById(int offerId);

    QAbstractListModel* getStandingsModel() const;
    QAbstractListModel* getCurrentTeamPlayersModel() const;
    QAbstractListModel* getCurrentTeamRecentMatchesModel() const;
    QAbstractListModel* getCurrentTeamUpcomingMatchesModel() const;
    QAbstractListModel* getPendingTransferOffersModel() const;
    TeamSeasonStatsObject* getCurrentTeamSeasonStatsObject() const;
    DashboardStateObject* getDashboardState() const;
    QAbstractListModel* getDashboardUpcomingMatchesModel() const;
    ShellStateObject* getShellState() const;
    InteractionStateObject* getInteractionState() const;
    EditableLineupStateObject* getEditableLineupState() const;
    EditableLineupSlotsModel* getEditableLineupSlotsModel() const;
    EditableLineupRosterModel* getEditableLineupRosterModel() const;

    Q_INVOKABLE QVariantList getStandingsTable() const;
    Q_INVOKABLE QVariantMap getCurrentTeamSeasonStats() const;
    Q_INVOKABLE QVariantList getCurrentTeamMatches() const;
    Q_INVOKABLE QVariantList getCurrentTeamUpcomingMatches(int count = 5) const;
    Q_INVOKABLE QVariantList getCurrentTeamPlayers() const;
    Q_INVOKABLE bool ensureEditableLineupReady();
    Q_INVOKABLE bool assignEditableLineupPlayerToSlot(int playerId, int slotIndex);
    Q_INVOKABLE bool clearEditableLineupSlot(int slotIndex);
    Q_INVOKABLE bool unassignEditableLineupPlayer(int playerId);

    Q_INVOKABLE QVariantMap getPlayerDetails(int playerId) const;
    Q_INVOKABLE QVariantMap getMatchReportDetails(qulonglong matchId) const;

signals:
    void gameStateChanged();
    void lastErrorChanged();

private:
    std::unique_ptr<Game> game;
    StandingsTableModel standingsModel;
    TeamPlayersModel teamPlayersModel;
    TeamRecentMatchesModel teamRecentMatchesModel;
    TeamUpcomingMatchesModel teamUpcomingMatchesModel;
    PendingTransferOffersModel pendingTransferOffersModel;
    TeamSeasonStatsObject currentTeamSeasonStatsObject;
    DashboardStateObject dashboardStateObject;
    DashboardUpcomingMatchesModel dashboardUpcomingMatchesModel;
    ShellStateObject shellStateObject;
    InteractionStateObject interactionStateObject;
    EditableLineupStateObject editableLineupStateObject;
    EditableLineupSlotsModel editableLineupSlotsModel;
    EditableLineupRosterModel editableLineupRosterModel;
    LeagueId selectedLeagueId = 0;
    TeamId selectedTeamId = 0;
    bool gameStarted = false;
    QString managerName;
    QString lastError;
    std::optional<EditableLineup> editableLineup;

    Game* ensureGame();
    const Game* ensureGame() const;
    LeagueContext* resolveLeagueContext(LeagueId leagueId);
    const LeagueContext* resolveLeagueContext(LeagueId leagueId) const;
    const League* resolveLeague(LeagueId leagueId) const;
    bool ensureSelectedTeamContext();
    bool hasValidSelectedTeam();
    bool hasValidSelectedTeam() const;
    bool hasValidLeagueSelection() const;
    bool startNewGameInternal(LeagueId leagueId, TeamId teamId, const QString& managerName);
    void setLastError(const QString& errorMessage);
    void refreshStandingsModel();
    void refreshCurrentTeamPlayersModel();
    void refreshCurrentTeamRecentMatchesModel();
    void refreshCurrentTeamUpcomingMatchesModel();
    void refreshPendingTransferOffersModel();
    void refreshCurrentTeamSeasonStatsObject();
    void refreshDashboardState();
    void refreshDashboardUpcomingMatchesModel();
    void refreshShellStateObject();
    void refreshInteractionStateObject();
    void refreshEditableLineup();
    void clearEditableLineupData();
    void refreshEditableLineupViews();
    void refreshEditableLineupStateObject();
    void refreshEditableLineupSlotsModel();
    void refreshEditableLineupRosterModel();
    int getSelectedTeamPlayerCount() const;
    int getExpectedEditableLineupSlotCount() const;
    const EditableLineup* resolveEditableLineup() const;
    void publishGameStateChanged();

    QString formatDate(const Date& date) const;
    QString formatGameState(GameState state) const;
    QString formatTransferOfferExpiryPolicy(TransferOfferExpiryPolicy policy) const;

    QString formatFormation(FormationId formation) const;
    QString formatSlotRole(FormationSlotRole role) const;
    QString formatPositionShortCode(const Footballer& player) const;
    QVariantList buildLineupViewRows(const TeamSheet& teamSheet, const League* league) const;
    QVariantList buildLineupViewRows(const MatchLineupSnapshot& snapshot, const League* league) const;

    QVariantMap toNextMatchMap(const FixtureMatchPreview& preview) const;
    QVariantMap toTeamStatsMap(const TeamSeasonStats& stats) const;
    QVariantMap toMatchRecordMap(const MatchRecord& record) const;
    QVariantMap toPlayerMap(const Footballer& player) const;
    QVariantMap toPlayerDetailsMap(const Footballer& player) const;
    QVariantMap toPlayerSeasonStatsMap(const PlayerSeasonStats& stats) const;
    QVariantMap toMatchReportDetailsMap(const MatchReport& report, const League* league) const;

    QVariantMap toPreMatchInteractionMap(const PreMatchInteraction& interaction) const;
    QVariantMap toPostMatchInteractionMap(const PostMatchInteraction& interaction) const;
    QVariantMap toTransferOfferInteractionMap(const TransferOfferDecisionInteraction& interaction) const;
    QVariantMap toPendingTransferOfferMap(const TransferOffer& offer) const;
};
