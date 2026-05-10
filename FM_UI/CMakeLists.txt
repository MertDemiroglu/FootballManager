cmake_minimum_required(VERSION 3.16)

project(FM_UI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_standard_project_setup()

qt_add_executable(FM_UI
    main.cpp
    GameFacade.cpp
    GameFacade.h
    StandingsTableModel.cpp
    StandingsTableModel.h
    TeamPlayersModel.cpp
    TeamPlayersModel.h
    TeamRecentMatchesModel.cpp
    TeamRecentMatchesModel.h
    TeamUpcomingMatchesModel.cpp
    TeamUpcomingMatchesModel.h
    PendingTransferOffersModel.cpp
    PendingTransferOffersModel.h
    TeamSeasonStatsObject.cpp
    TeamSeasonStatsObject.h
    DashboardStateObject.cpp
    DashboardStateObject.h
    DashboardNextMatchObject.cpp
    DashboardNextMatchObject.h
    DashboardTeamStatsObject.cpp
    DashboardTeamStatsObject.h
    DashboardStandingsRowObject.cpp
    DashboardStandingsRowObject.h
    DashboardUpcomingMatchesModel.cpp
    DashboardUpcomingMatchesModel.h
    ShellStateObject.cpp
    ShellStateObject.h
    InteractionStateObject.cpp
    InteractionStateObject.h
    PreMatchInteractionObject.cpp
    PreMatchInteractionObject.h
    PostMatchInteractionObject.cpp
    PostMatchInteractionObject.h
    TransferOfferInteractionObject.cpp
    TransferOfferInteractionObject.h
    EditableLineupStateObject.cpp
    EditableLineupStateObject.h
    EditableLineupSlotsModel.cpp
    EditableLineupSlotsModel.h
    EditableLineupRosterModel.cpp
    EditableLineupRosterModel.h
)

qt_add_qml_module(FM_UI
    URI FM_UI
    VERSION 1.0
    QML_FILES
        Main.qml
        HomeView.qml
        NewGameView.qml
        AppIcon.qml
        DashboardView.qml
        FootballPitchSurface.qml
        LineupPlayerToken.qml
        MatchPitchPreview.qml
        MatchTeamBadge.qml
        TeamVisuals.js
        PreMatchScreen.qml
        PostMatchScreen.qml
        TransferOffersView.qml
        StandingsView.qml
        TeamView.qml
        LineupEditorScreen.qml
        PlayerDetailsDialog.qml
        UiHelpers.js
        InteractionModalShell.qml
        MatchDetailDialog.qml
        LineupSlotRow.qml
        TeamLineupCard.qml
        MatchLineupSection.qml
        LineupEditorView.qml
        LineupPitchPanel.qml
        LineupSlotCard.qml
        LineupRosterPanel.qml
        LineupRosterRow.qml
        LineupStartingXISection.qml
        LineupStartingXIRow.qml
        PositionBadge.qml
        ConditionBadge.qml
        TransferOfferDialog.qml
    RESOURCES
        assets/icons/standings.svg
        assets/icons/team.svg
        assets/icons/lineup.svg
        assets/icons/transfers.svg
        assets/icons/fixtures.svg
        assets/icons/calendar.svg
        assets/icons/settings.svg
        assets/icons/main-menu.svg
        assets/icons/trophy.svg
        assets/icons/star.svg
        assets/icons/record.svg
        assets/icons/ball.svg
        assets/icons/form.svg
        assets/icons/play.svg
        assets/icons/pause.svg
        assets/icons/home.svg
        assets/icons/away.svg
        assets/icons/stats.svg
        assets/icons/chevron-right.svg
)

qt_add_executable(game_facade_smoke
    GameFacadeSmoke.cpp
    GameFacade.cpp
    GameFacade.h
    StandingsTableModel.cpp
    StandingsTableModel.h
    TeamPlayersModel.cpp
    TeamPlayersModel.h
    TeamRecentMatchesModel.cpp
    TeamRecentMatchesModel.h
    TeamUpcomingMatchesModel.cpp
    TeamUpcomingMatchesModel.h
    PendingTransferOffersModel.cpp
    PendingTransferOffersModel.h
    TeamSeasonStatsObject.cpp
    TeamSeasonStatsObject.h
    DashboardStateObject.cpp
    DashboardStateObject.h
    DashboardNextMatchObject.cpp
    DashboardNextMatchObject.h
    DashboardTeamStatsObject.cpp
    DashboardTeamStatsObject.h
    DashboardStandingsRowObject.cpp
    DashboardStandingsRowObject.h
    DashboardUpcomingMatchesModel.cpp
    DashboardUpcomingMatchesModel.h
    ShellStateObject.cpp
    ShellStateObject.h
    InteractionStateObject.cpp
    InteractionStateObject.h
    PreMatchInteractionObject.cpp
    PreMatchInteractionObject.h
    PostMatchInteractionObject.cpp
    PostMatchInteractionObject.h
    TransferOfferInteractionObject.cpp
    TransferOfferInteractionObject.h
    EditableLineupStateObject.cpp
    EditableLineupStateObject.h
    EditableLineupSlotsModel.cpp
    EditableLineupSlotsModel.h
    EditableLineupRosterModel.cpp
    EditableLineupRosterModel.h
)

set_target_properties(game_facade_smoke PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(FM_UI
    PRIVATE
        Qt6::Core
        Qt6::Quick
        fm_core
)

target_link_libraries(game_facade_smoke
    PRIVATE
        Qt6::Core
        fm_core
)
