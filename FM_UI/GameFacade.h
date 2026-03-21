#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "Types.h"

class Game;
class Footballer;

struct Date;
struct FixtureMatchPreview;
struct StandingsEntry;
struct TeamSeasonStats;
struct MatchRecord;
struct PlayerSeasonStats;

enum class GameState;

class GameFacade : public QObject {
    Q_OBJECT
        Q_DISABLE_COPY_MOVE(GameFacade)

public:
    explicit GameFacade(QObject* parent = nullptr);
    ~GameFacade();

    Q_INVOKABLE QVariantList getTeamSelectionList() const;
    Q_INVOKABLE bool startNewGame(int teamId, const QString& managerName);
    Q_INVOKABLE bool hasStartedGame() const;

    Q_INVOKABLE QString getCurrentDateText() const;
    Q_INVOKABLE QString getCurrentStateText() const;
    Q_INVOKABLE QString getSelectedTeamName() const;
    Q_INVOKABLE int getSelectedTeamId() const;
    Q_INVOKABLE QString getManagerName() const;

    Q_INVOKABLE QVariantMap getDashboard() const;

    Q_INVOKABLE bool advanceOneDay();
    Q_INVOKABLE bool advanceDays(int count);

    Q_INVOKABLE QVariantList getStandingsTable() const;
    Q_INVOKABLE QVariantMap getCurrentTeamSeasonStats() const;
    Q_INVOKABLE QVariantList getCurrentTeamMatches() const;
    Q_INVOKABLE QVariantList getCurrentTeamUpcomingMatches(int count = 5) const;
    Q_INVOKABLE QVariantList getCurrentTeamPlayers() const;

    Q_INVOKABLE QVariantMap getPlayerDetails(int playerId) const;

signals:
    void gameStateChanged();

private:
    std::unique_ptr<Game> game;
    TeamId selectedTeamId = 0;
    bool gameStarted = false;
    QString managerName;

    Game* ensureGame();
    const Game* ensureGame() const;
    bool hasValidSelectedTeam() const;

    QString formatDate(const Date& date) const;
    QString formatGameState(GameState state) const;

    QVariantMap toNextMatchMap(const FixtureMatchPreview& preview) const;
    QVariantMap toStandingsRowMap(const StandingsEntry& entry, int position) const;
    QVariantMap toTeamStatsMap(const TeamSeasonStats& stats) const;
    QVariantMap toMatchRecordMap(const MatchRecord& record) const;
    QVariantMap toPlayerMap(const Footballer& player) const;
    QVariantMap toPlayerDetailsMap(const Footballer& player) const;
    QVariantMap toPlayerSeasonStatsMap(const PlayerSeasonStats& stats) const;
};