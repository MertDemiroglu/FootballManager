cmake_minimum_required(VERSION 3.16)

project(fm_core LANGUAGES CXX)

add_library(fm_core STATIC
    src/Game.cpp
    src/roster/Contract.cpp
    src/Date.cpp
    src/DateUtils.cpp
    src/roster/Defender.cpp
    src/events/DomainEventPublisher.cpp
    src/EventQueue.cpp
    src/roster/Formation.cpp
    src/competition/FixtureGenerator.cpp
    src/roster/Footballer.cpp
    src/roster/FootballerFactory.cpp
    src/roster/Forward.cpp
    src/GameInteraction.cpp
    src/roster/HeadCoach.cpp
    src/roster/Goalkeeper.cpp
    src/InteractionManager.cpp
    src/competition/League.cpp
    src/World.cpp
    src/LeagueContext.cpp
    src/competition/LeagueRules.cpp
    src/competition/MatchScheduler.cpp
    src/MatchSimulation.cpp
    src/PlayerConditionState.cpp
    src/PlayerConditionService.cpp
    src/PreMatchInteraction.cpp
    src/PostMatchInteraction.cpp
    src/PlayMatchCommandHandler.cpp
    src/roster/PlayerRoleFit.cpp
    src/roster/PlayerPosition.cpp
    src/roster/PlayerPositionBucket.cpp
    src/roster/Midfielder.cpp
    src/RosterLoader.cpp
    src/competition/SeasonPlan.cpp
    src/roster/Team.cpp
    src/TeamSelectionService.cpp
    src/TeamSheet.cpp
    src/TransferOfferDecisionInteraction.cpp
    src/transfer/TransferOfferService.cpp
    src/transfer/TransferRoom.cpp
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
