cmake_minimum_required(VERSION 3.16)

project(fm_core LANGUAGES CXX)

add_library(fm_core STATIC
    src/Game.cpp
    src/Contract.cpp
    src/Date.cpp
    src/DateUtils.cpp
    src/Defender.cpp
    src/DomainEventPublisher.cpp
    src/EventQueue.cpp
    src/Formation.cpp
    src/FixtureGenerator.cpp
    src/Footballer.cpp
    src/FootballerFactory.cpp
    src/Forward.cpp
    src/GameInteraction.cpp
    src/HeadCoach.cpp
    src/Goalkeeper.cpp
    src/InteractionManager.cpp
    src/League.cpp
    src/World.cpp
    src/LeagueContext.cpp
    src/LeagueRules.cpp
    src/MatchScheduler.cpp
    src/MatchSimulation.cpp
    src/PlayerConditionState.cpp
    src/PlayerConditionService.cpp
    src/PreMatchInteraction.cpp
    src/PostMatchInteraction.cpp
    src/PlayMatchCommandHandler.cpp
    src/PlayerRoleFit.cpp
    src/PlayerPosition.cpp
    src/PlayerPositionBucket.cpp
    src/Midfielder.cpp
    src/RosterLoader.cpp
    src/SeasonPlan.cpp
    src/Team.cpp
    src/TeamSelectionService.cpp
    src/TeamSheet.cpp
    src/TransferOfferDecisionInteraction.cpp
    src/TransferOfferService.cpp
    src/TransferRoom.cpp
    src/User.cpp
)

add_executable(fm_runner
    src/main.cpp
)

target_link_libraries(fm_runner PRIVATE fm_core)

target_compile_features(fm_core PUBLIC cxx_std_17)
target_compile_features(fm_runner PRIVATE cxx_std_17)

target_include_directories(fm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
