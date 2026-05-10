#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "GameFacade.h"
#include "fm/core/Game.h"
#include "fm/match/EditableLineup.h"
#include "fm/match/TeamSheet.h"
#include "fm/roster/Defender.h"
#include "fm/roster/Forward.h"
#include "fm/roster/Formation.h"
#include "fm/roster/FormationPitchLayout.h"
#include "fm/roster/Goalkeeper.h"
#include "fm/roster/Midfielder.h"
#include "fm/roster/Team.h"

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

    std::unique_ptr<Footballer> makePlayer(int index) {
        const std::string name = "Smoke Player " + std::to_string(index);
        if (index == 0) {
            return std::make_unique<Goalkeeper>(name, "Smoke FC", 24, 60, 60, 60, 60, 60);
        }
        if (index <= 4) {
            return std::make_unique<Defender>(name, PlayerPosition::CenterBack, "Smoke FC", 24, 60, 60, 60, 60, 60);
        }
        if (index <= 8) {
            return std::make_unique<Midfielder>(name, PlayerPosition::CentralMidfielder, "Smoke FC", 24, 60, 60, 60, 60, 60);
        }
        return std::make_unique<Forward>(name, PlayerPosition::Striker, "Smoke FC", 24, 60, 60, 60, 60, 60);
    }

    void expectEditableLineupTeamSheetExport() {
        Team team(91001, "Smoke FC");
        for (int i = 0; i < 11; ++i) {
            team.addPlayer(makePlayer(i));
        }

        EditableLineup lineup(team, FormationId::FourThreeThree);
        expect(!lineup.toTeamSheetIfComplete().has_value(),
            "incomplete editable lineup should not export to TeamSheet");

        const auto& players = team.getPlayers();
        for (std::size_t i = 0; i < lineup.getSlots().size(); ++i) {
            expect(lineup.assignPlayerToSlot(players.at(i)->getId(), i),
                "assigning smoke player to editable lineup slot should succeed");
        }

        const std::optional<TeamSheet> exported = lineup.toTeamSheetIfComplete();
        expect(exported.has_value(), "complete editable lineup should export to TeamSheet");
        expect(exported->teamId == team.getId(), "exported TeamSheet should keep team id");
        expect(exported->formation == FormationId::FourThreeThree,
            "exported TeamSheet should keep editable formation");
        expect(exported->startingPlayerIds.size() == 11,
            "exported TeamSheet should contain 11 starting player ids");
        expect(exported->startingAssignments.size() == 11,
            "exported TeamSheet should contain 11 slot assignments");
    }
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    try {
        qDebug() << "[Smoke] Creating facade...";
        expectSupportedFormationPitchLayouts();
        expectEditableLineupTeamSheetExport();
        {
            Game gameWithoutPreMatch;
            TeamSheet sheet;
            sheet.teamId = 1;
            expect(!gameWithoutPreMatch.replacePendingPreMatchTeamSheetForTeam(1, sheet),
                "pending pre-match sheet replacement should fail safely without active pre-match");
        }
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
        const QVariantMap dashboard = facade.getDashboard();
        expect(!dashboard.isEmpty(), "dashboard should not be empty");
        expect(!dashboard.value(QStringLiteral("selectedTeamPrimaryColor")).toString().isEmpty(),
            "dashboard should expose selected team primary color");
        expect(!dashboard.value(QStringLiteral("selectedTeamSecondaryColor")).toString().isEmpty(),
            "dashboard should expose selected team secondary color");
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
        expect(!nextMatch.value(QStringLiteral("homePrimaryColor")).toString().isEmpty(),
            "dashboard next match should expose home primary color");
        expect(!nextMatch.value(QStringLiteral("awayPrimaryColor")).toString().isEmpty(),
            "dashboard next match should expose away primary color");

        expect(!facade.applyEditableLineupToActivePreMatch(),
            "applying editable lineup should fail safely when no pre-match is active");
        expect(facade.autoSelectEditableLineup(),
            "auto select before managed pre-match should produce a complete editable lineup");

        bool sawTeamMatchHistory = false;
        bool sawManagedPreMatch = false;
        qDebug() << "[Smoke] Advancing days until team match history appears...";
        expect(facade.resumeSimulation(), "smoke should resume simulation before advancing days");
        for (int day = 0; day < 370; ++day) {
            expect(facade.advanceOneDay(), "advanceOneDay should succeed after start");
            const QString activeKind = facade.getActiveInteractionKind();
            if (activeKind == QStringLiteral("transfer_offer")) {
                expect(facade.rejectActiveTransferOffer(),
                    "smoke should resolve transfer offers while advancing toward a managed match");
                expect(facade.resumeSimulation(),
                    "smoke should resume after resolving a transfer offer");
                continue;
            }
            if (activeKind == QStringLiteral("post_match")) {
                expect(facade.resolveActiveInteraction(),
                    "smoke should resolve post-match interactions while advancing");
                expect(facade.resumeSimulation(),
                    "smoke should resume after resolving a post-match interaction");
                continue;
            }
            if (activeKind == QStringLiteral("pre_match")) {
                sawManagedPreMatch = true;
                expect(facade.applyEditableLineupToActivePreMatch(),
                    "complete editable lineup should apply to active pre-match");
                const QVariantMap preMatch = facade.getActivePreMatchInteraction();
                const bool managedHome = preMatch.value(QStringLiteral("homeTeamId")).toInt() == facade.getSelectedTeamId();
                const QVariantList managedLineup = managedHome
                    ? preMatch.value(QStringLiteral("homeLineup")).toList()
                    : preMatch.value(QStringLiteral("awayLineup")).toList();
                expect(managedLineup.size() == 11,
                    "active pre-match should expose 11 managed lineup rows after apply");
                expect(!preMatch.value(QStringLiteral("homePrimaryColor")).toString().isEmpty(),
                    "active pre-match should expose home primary color");
                expect(!preMatch.value(QStringLiteral("awayPrimaryColor")).toString().isEmpty(),
                    "active pre-match should expose away primary color");
                expect(!preMatch.value(QStringLiteral("homeAverageOverallText")).toString().isEmpty(),
                    "active pre-match should expose home average overall text");
                expect(!preMatch.value(QStringLiteral("awayAverageOverallText")).toString().isEmpty(),
                    "active pre-match should expose away average overall text");
                expect(facade.playActiveMatch(), "playActiveMatch should succeed for active pre-match");
                if (facade.getActiveInteractionKind() == QStringLiteral("post_match")) {
                    expect(facade.resolveActiveInteraction(),
                        "post-match interaction should resolve after managed match smoke");
                }
            }
            if (!facade.getCurrentTeamMatches().isEmpty()) {
                sawTeamMatchHistory = true;
                break;
            }
        }

        expect(sawManagedPreMatch,
            "smoke should encounter a managed-team pre-match interaction");
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
