cmake_minimum_required(VERSION 3.16)

project(fm_core LANGUAGES CXX)

add_library(fm_core STATIC
    Game.cpp
    Contract.cpp
    Date.cpp
    DateUtils.cpp
    Defender.cpp
    EventQueue.cpp
    FixtureGenerator.cpp
    Footballer.cpp
    FootballerFactory.cpp
    Forward.cpp
    Goalkeeper.cpp
    League.cpp
    MatchEvent.cpp
    MatchScheduler.cpp
    LeagueRules.cpp
    Midfielder.cpp
    SeasonPlan.cpp
    RosterLoader.cpp
    Team.cpp
    TransferOfferEvent.cpp
    TransferRoom.cpp
    User.cpp)

    add_executable(fm_runner
    main.cpp
)

target_link_libraries(fm_runner PRIVATE fm_core)
target_compile_features(fm_runner PRIVATE cxx_std_17)

target_compile_features(fm_core PRIVATE cxx_std_17)
