cmake_minimum_required(VERSION 3.16)

project(FM_UI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_standard_project_setup()

qt_add_executable(FM_UI
    main.cpp
)

qt_add_qml_module(FM_UI
    URI FM_UI
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(FM_UI
    PRIVATE
        Qt6::Core
        Qt6::Quick
        fm_core
)