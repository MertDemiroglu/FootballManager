cmake_minimum_required(VERSION 3.16)

project(fm_core LANGUAGES CXX)

add_library(fm_core STATIC
    Game.cpp
    Contract.cpp
    Date.cpp
    DateUtils.cpp
    Defender.cpp
    DomainEventPublisher.cpp
    EventQueue.cpp
    Formation.cpp
    FixtureGenerator.cpp
    Footballer.cpp
    FootballerFactory.cpp
    Forward.cpp
    GameInteraction.cpp
    HeadCoach.cpp
    Goalkeeper.cpp
    InteractionManager.cpp
    League.cpp
    World.cpp
    LeagueContext.cpp
    LeagueRules.cpp 
    MatchScheduler.cpp
    MatchSimulation.cpp
    PreMatchInteraction.cpp
    PostMatchInteraction.cpp
    PlayMatchCommandHandler.cpp
    PlayerRoleFit.cpp
    PlayerPosition.cpp
    PlayerPositionBucket.cpp
    Midfielder.cpp
    RosterLoader.cpp
    SeasonPlan.cpp
    Team.cpp    
    TeamSelectionService.cpp
    TeamSheet.cpp
    TransferOfferDecisionInteraction.cpp
    TransferOfferService.cpp
    TransferRoom.cpp
    User.cpp)

    add_executable(fm_runner
    main.cpp
)

target_link_libraries(fm_runner PRIVATE fm_core)

target_compile_features(fm_runner PRIVATE cxx_std_17)

target_include_directories(fm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
