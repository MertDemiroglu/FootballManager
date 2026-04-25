#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <exception>
#include <optional>
#include <stdexcept>

#include "GameFacade.h"
#include "fm/roster/Formation.h"
#include "fm/roster/FormationPitchLayout.h"

namespace {
    void expect(bool condition, const QString& message) {
        if (!condition) {
            throw std::runtime_error(message.toStdString());
        }
    }

    void expectSupportedFormationPitchLayouts() {
        for (FormationId formationId : getSupportedFormationIds()) {
            const std::vector<FormationSlotRole>& slotTemplate = getFormationSlotTemplate(formationId);
            expect(slotTemplate.size() == 11, "supported formation template should have 11 slots");
            for (std::size_t slotIndex = 0; slotIndex < slotTemplate.size(); ++slotIndex) {
                const std::optional<FormationPitchCoordinate> coordinate =
                    getFormationPitchCoordinate(formationId, slotIndex, slotTemplate[slotIndex]);
                expect(coordinate.has_value(), "supported formation should have pitch coordinates");
                expect(coordinate->x >= 0.0 && coordinate->x <= 1.0,
                    "pitch coordinate x should be normalized");
                expect(coordinate->y >= 0.0 && coordinate->y <= 1.0,
                    "pitch coordinate y should be normalized");
                expect(coordinate->displayOrder == static_cast<int>(slotIndex),
                    "display order should match formation slot order");
            }
        }
    }
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    try {
        qDebug() << "[Smoke] Creating facade...";
        expectSupportedFormationPitchLayouts();
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
        const QVariantList slotRows = facade.getEditableLineupSlotsModel()->rows();
        expect(!slotRows.isEmpty(), "editable lineup slot rows should be exposed");
        for (const QVariant& slotValue : slotRows) {
            const QVariantMap slotRow = slotValue.toMap();
            const double pitchX = slotRow.value(QStringLiteral("pitchX"), -1.0).toDouble();
            const double pitchY = slotRow.value(QStringLiteral("pitchY"), -1.0).toDouble();
            expect(pitchX >= 0.0 && pitchX <= 1.0, "slot row pitchX should be normalized");
            expect(pitchY >= 0.0 && pitchY <= 1.0, "slot row pitchY should be normalized");
            expect(slotRow.contains(QStringLiteral("displayOrder")),
                "slot row should expose displayOrder");
        }
        expect(facade.getEditableLineupRosterModel()->rowCount() == players.size(),
            "editable lineup roster should match current team players");
        expect(facade.getEditableLineupState()->slotCount() == 11,
            "editable lineup should expose 11 slots");
        expect(facade.getEditableLineupState()->assignedCount() == 0,
            "editable lineup should start with no assigned players");

        expect(players.size() >= 2, "lineup edit smoke needs at least two players");
        expect(facade.getEditableLineupSlotsModel()->rowCount() >= 2,
            "lineup edit smoke needs at least two slots");

        const int firstPlayerId = players.at(0).toMap().value(QStringLiteral("playerId")).toInt();
        const int secondPlayerId = players.at(1).toMap().value(QStringLiteral("playerId")).toInt();

        qDebug() << "[Smoke] Checking editable lineup supported formations and formation change...";
        const QVariantList supportedFormations = facade.getEditableLineupSupportedFormations();
        expect(supportedFormations.size() >= 2,
            "editable lineup should expose multiple supported formations");
        const int initialFormation = facade.getEditableLineupState()->formation();
        expect(!facade.getEditableLineupState()->formationText().isEmpty(),
            "initial editable lineup formation text should exist");

        QVariantMap targetFormationItem;
        for (const QVariant& itemValue : supportedFormations) {
            const QVariantMap item = itemValue.toMap();
            const int candidateFormation = item.value(QStringLiteral("formationId")).toInt();
            if (candidateFormation == initialFormation) {
                continue;
            }

            targetFormationItem = item;
            break;
        }
        expect(!targetFormationItem.isEmpty(),
            "formation change smoke should find another supported formation");

        qDebug() << "[Smoke] Checking explicit auto select...";
        expect(facade.autoSelectEditableLineup(),
            "auto-selecting editable lineup should succeed");
        expect(facade.getEditableLineupState()->assignedCount() == 11,
            "auto select should fill 11 slots when enough players exist");

        const int rosterCountBeforeFormationChange = facade.getEditableLineupRosterModel()->rowCount();
        const int targetFormation = targetFormationItem.value(QStringLiteral("formationId")).toInt();
        const QString targetFormationText = targetFormationItem.value(QStringLiteral("formationText")).toString();
        expect(facade.changeEditableLineupFormation(targetFormation),
            "changing editable lineup formation should succeed");
        expect(facade.getEditableLineupState()->formation() == targetFormation,
            "editable lineup state formation should update after formation change");
        expect(facade.getEditableLineupState()->formationText() == targetFormationText,
            "editable lineup state formation text should update after formation change");
        expect(facade.getEditableLineupState()->slotCount() == facade.getEditableLineupSlotsModel()->rowCount(),
            "editable lineup state slot count should match slots model after formation change");
        expect(facade.getEditableLineupState()->slotCount()
                == static_cast<int>(getFormationSlotTemplate(static_cast<FormationId>(targetFormation)).size()),
            "editable lineup slot count should match selected formation template");
        expect(facade.getEditableLineupRosterModel()->rowCount() == rosterCountBeforeFormationChange,
            "editable lineup roster count should remain unchanged after formation change");
        expect(facade.getEditableLineupState()->assignedCount() == 0,
            "formation change should clear all editable lineup assignments");

        expect(facade.autoSelectEditableLineup(),
            "auto select after formation change should succeed");
        expect(facade.getEditableLineupState()->formation() == targetFormation,
            "auto select should keep the selected editable formation");
        expect(facade.getEditableLineupState()->assignedCount() == 11,
            "auto select after formation change should fill the selected formation");

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
        expect(facade.unassignEditableLineupPlayer(secondPlayerId),
            "unassigning selected second player should succeed");
        expect(facade.getEditableLineupSlotsModel()->data(
            facade.getEditableLineupSlotsModel()->index(0, 0),
            EditableLineupSlotsModel::AssignedPlayerIdRole).toInt() == 0,
            "unassigning second player should clear their current slot");

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
