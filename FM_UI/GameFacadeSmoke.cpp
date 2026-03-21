#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <exception>
#include <stdexcept>

#include "GameFacade.h"

namespace {
    void expect(bool condition, const QString& message) {
        if (!condition) {
            throw std::runtime_error(message.toStdString());
        }
    }
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    try {
        qDebug() << "[Smoke] Creating facade...";
        GameFacade facade;

        qDebug() << "[Smoke] Reading team selection...";
        const QVariantList teamSelection = facade.getTeamSelectionList();
        expect(!teamSelection.isEmpty(), "team selection list should not be empty");

        qDebug() << "[Smoke] Reject invalid team id...";
        expect(!facade.startNewGame(-1, QStringLiteral("Invalid")),
            "invalid team id should be rejected");

        const QVariantMap firstTeam = teamSelection.first().toMap();
        const int teamId = firstTeam.value(QStringLiteral("teamId")).toInt();
        expect(teamId > 0, "valid team id should be positive");

        qDebug() << "[Smoke] Starting game with teamId =" << teamId;
        expect(facade.startNewGame(teamId, QStringLiteral("Facade Test Manager")),
            "valid team should start the game");

        expect(facade.hasStartedGame(), "started flag should be true");
        expect(!facade.getDashboard().isEmpty(), "dashboard should not be empty");
        expect(!facade.getSelectedTeamName().isEmpty(), "selected team name should not be empty");
        expect(facade.getSelectedTeamId() > 0, "selected team id should resolve in fresh game");
        expect(facade.getManagerName() == QStringLiteral("Facade Test Manager"),
            "manager name should roundtrip");
        expect(facade.getLastError().isEmpty(), "last error should be cleared after successful start");

        qDebug() << "[Smoke] Reading standings...";
        const QVariantList standings = facade.getStandingsTable();
        expect(standings.size() == teamSelection.size(),
            "standings table size should equal team count");

        qDebug() << "[Smoke] Reading team players...";
        const QVariantList players = facade.getCurrentTeamPlayers();
        expect(!players.isEmpty(), "current team players should not be empty");

        const int playerId = players.first().toMap().value(QStringLiteral("playerId")).toInt();
        qDebug() << "[Smoke] Reading player details for playerId =" << playerId;
        const QVariantMap playerDetails = facade.getPlayerDetails(playerId);
        expect(playerDetails.value(QStringLiteral("hasPlayer")).toBool(),
            "player details should resolve for a valid player");

        qDebug() << "[Smoke] Reading next match from dashboard...";
        const QVariantMap nextMatch = facade.getDashboard().value(QStringLiteral("nextMatch")).toMap();
        expect(nextMatch.value(QStringLiteral("hasNextMatch")).toBool(),
            "dashboard next match should exist after boot");

        bool sawTeamMatchHistory = false;
        qDebug() << "[Smoke] Advancing days until team match history appears...";
        for (int day = 0; day < 200; ++day) {
            expect(facade.advanceOneDay(), "advanceOneDay should succeed after start");
            if (!facade.getCurrentTeamMatches().isEmpty()) {
                sawTeamMatchHistory = true;
                break;
            }
        }

        expect(sawTeamMatchHistory,
            "team match history should become non-empty as the season advances");
        expect(!facade.getCurrentTeamSeasonStats().isEmpty(),
            "team season stats should be available");

        qDebug() << "[Smoke] PASS";
        return 0;
    }
    catch (const std::exception& ex) {
        qCritical() << "[Smoke][FAIL]" << ex.what();
        return 1;
    }
    catch (...) {
        qCritical() << "[Smoke][FAIL] Unknown exception";
        return 1;
    }
}