cmake_minimum_required(VERSION 3.16)

project(fm_core LANGUAGES CXX)

add_library(fm_core STATIC
    Game.cpp
    Contract.cpp
    Date.cpp
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
    Midfielder.cpp
    Team.cpp
    TransferOfferEvent.cpp
    TransferRoom.cpp
    User.cpp
 "FixtureGenerator.cpp" "FixtureGenerator.h")
