cmake_minimum_required(VERSION 3.16)

project(FM_UI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_standard_project_setup()

qt_add_executable(FM_UI
    main.cpp
    GameFacade.cpp
    GameFacade.h
)

qt_add_qml_module(FM_UI
    URI FM_UI
    VERSION 1.0
    QML_FILES
        Main.qml
        NewGameView.qml
        DashboardView.qml
        StandingsView.qml
        TeamView.qml
        PlayerDetailsDialog.qml
)

qt_add_executable(game_facade_smoke
    GameFacadeSmoke.cpp
    GameFacade.cpp
    GameFacade.h
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