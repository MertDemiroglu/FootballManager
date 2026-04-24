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
        const int leagueId = firstTeam.value(QStringLiteral("leagueId")).toInt();
        const int teamId = firstTeam.value(QStringLiteral("teamId")).toInt();
        expect(leagueId > 0, "valid league id should be positive");
        expect(teamId > 0, "valid team id should be positive");

        qDebug() << "[Smoke] Starting game with leagueId =" << leagueId << "teamId =" << teamId;
        expect(facade.startNewGameForLeagueTeam(leagueId, teamId, QStringLiteral("Facade Test Manager")),
            "valid league/team should start the game");

        expect(facade.hasStartedGame(), "started flag should be true");
        expect(!facade.getDashboard().isEmpty(), "dashboard should not be empty");
        expect(facade.getSelectedLeagueId() == leagueId, "selected league id should roundtrip");
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

        qDebug() << "[Smoke] Preparing editable lineup...";
        expect(facade.ensureEditableLineupReady(),
            "editable lineup should be ready for the managed team");
        expect(facade.getEditableLineupState()->hasLineup(),
            "editable lineup state should report a lineup");
        expect(facade.getEditableLineupSlotsModel()->rowCount() > 0,
            "editable lineup slots should be populated");
        expect(facade.getEditableLineupRosterModel()->rowCount() == players.size(),
            "editable lineup roster should match current team players");

        expect(players.size() >= 2, "lineup edit smoke needs at least two players");
        expect(facade.getEditableLineupSlotsModel()->rowCount() >= 2,
            "lineup edit smoke needs at least two slots");

        const int firstPlayerId = players.at(0).toMap().value(QStringLiteral("playerId")).toInt();
        const int secondPlayerId = players.at(1).toMap().value(QStringLiteral("playerId")).toInt();

        qDebug() << "[Smoke] Checking editable lineup assign/move/replace semantics...";
        expect(facade.assignEditableLineupPlayerToSlot(firstPlayerId, 0),
            "assigning first player to slot 0 should succeed");
        expect(facade.assignEditableLineupPlayerToSlot(secondPlayerId, 1),
            "assigning second player to slot 1 should succeed");
        expect(facade.assignEditableLineupPlayerToSlot(firstPlayerId, 1),
            "assigning already-assigned first player to slot 1 should move and replace");
        expect(facade.getEditableLineupSlotsModel()->data(
            facade.getEditableLineupSlotsModel()->index(0, 0),
            EditableLineupSlotsModel::AssignedPlayerIdRole).toInt() == 0,
            "moving first player away from slot 0 should clear the old slot");
        expect(facade.getEditableLineupSlotsModel()->data(
            facade.getEditableLineupSlotsModel()->index(1, 0),
            EditableLineupSlotsModel::AssignedPlayerIdRole).toInt() == firstPlayerId,
            "target slot should contain moved first player");
        expect(facade.getEditableLineupRosterModel()->data(
            facade.getEditableLineupRosterModel()->index(1, 0),
            EditableLineupRosterModel::IsAssignedRole).toBool() == false,
            "replaced second player should become unassigned");

        expect(facade.assignEditableLineupPlayerToSlot(secondPlayerId, 0),
            "reassigning second player to slot 0 should succeed");
        expect(facade.swapEditableLineupSlots(0, 1),
            "swapping two occupied slots should succeed");
        expect(facade.getEditableLineupSlotsModel()->data(
            facade.getEditableLineupSlotsModel()->index(0, 0),
            EditableLineupSlotsModel::AssignedPlayerIdRole).toInt() == firstPlayerId,
            "occupied slot swap should put first player in slot 0");
        expect(facade.getEditableLineupSlotsModel()->data(
            facade.getEditableLineupSlotsModel()->index(1, 0),
            EditableLineupSlotsModel::AssignedPlayerIdRole).toInt() == secondPlayerId,
            "occupied slot swap should put second player in slot 1");

        expect(facade.clearEditableLineupSlot(0),
            "clearing slot 0 before empty-slot move should succeed");
        expect(facade.swapEditableLineupSlots(0, 1),
            "swapping an empty slot with an occupied slot should move the occupant");
        expect(facade.getEditableLineupSlotsModel()->data(
            facade.getEditableLineupSlotsModel()->index(0, 0),
            EditableLineupSlotsModel::AssignedPlayerIdRole).toInt() == secondPlayerId,
            "empty occupied swap should move second player into slot 0");
        expect(facade.getEditableLineupSlotsModel()->data(
            facade.getEditableLineupSlotsModel()->index(1, 0),
            EditableLineupSlotsModel::AssignedPlayerIdRole).toInt() == 0,
            "empty occupied swap should leave slot 1 empty");
        expect(!facade.swapEditableLineupSlots(1, 1),
            "same-slot swap should be a deterministic no-op failure");

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
