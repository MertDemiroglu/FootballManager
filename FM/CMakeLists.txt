cmake_minimum_required(VERSION 3.16)

project(fm_core LANGUAGES C CXX)

add_library(fm_core STATIC
    src/core/Game.cpp
    src/core/SaveScheduler.cpp
    src/finance/TeamFinance.cpp
    src/roster/Contract.cpp
    src/common/Date.cpp
    src/common/DateUtils.cpp
    src/roster/Defender.cpp
    src/events/DomainEventPublisher.cpp
    src/match/EventQueue.cpp
    src/roster/Formation.cpp
    src/roster/FormationPitchLayout.cpp
    src/competition/FixtureGenerator.cpp
    src/roster/Footballer.cpp
    src/roster/FootballerFactory.cpp
    src/roster/Forward.cpp
    src/roster/PlayerAttributes.cpp
    src/roster/PlayerOverallCalculator.cpp
    src/interaction/GameInteraction.cpp
    src/roster/HeadCoach.cpp
    src/roster/Goalkeeper.cpp
    src/interaction/InteractionManager.cpp
    src/competition/League.cpp
    src/core/World.cpp
    src/core/LeagueContext.cpp
    src/competition/LeagueRules.cpp
    src/competition/MatchScheduler.cpp
    src/match_engine/PitchGeometry.cpp
    src/match_engine/MatchEngine.cpp
    src/match_engine/MatchEngineReportAdapter.cpp
    src/match_engine/CoordinateMatchSimulator.cpp
    src/match_engine/MatchEngineInputBuilder.cpp
    src/match_engine/BallTrajectoryBuilder.cpp
    src/match_engine/InterceptionResolver.cpp
    src/match_engine/ContestResolver.cpp
    src/match_engine/TacticalZone.cpp
    src/match_engine/DefensiveResponsibility.cpp
    src/match_engine/PlayerIntentResolver.cpp
    src/match_engine/MovementResolver.cpp
    src/match_engine/TeamShapeModel.cpp
    src/match_engine/ActionPlan.cpp
    src/match_engine/ActionCandidateGenerator.cpp
    src/match_engine/ActionSelector.cpp
    src/match/MatchSimulation.cpp
    src/match/PlayerConditionState.cpp
    src/match/PlayerConditionService.cpp
    src/match/TeamSheetEvaluationService.cpp
    src/presentation/MatchPresentationBuilder.cpp
    src/presentation/TeamPresentationBuilder.cpp
    src/presentation/TeamSheetPresentationBuilder.cpp
    src/interaction/PreMatchInteraction.cpp
    src/interaction/PostMatchInteraction.cpp
    src/match/PlayMatchCommandHandler.cpp
    src/roster/PlayerRoleFit.cpp
    src/roster/PlayerPosition.cpp
    src/roster/PlayerPositionBucket.cpp
    src/roster/Midfielder.cpp
    src/data/RosterLoader.cpp
    src/data/RuntimeSaveValidator.cpp
    src/data/SavePolicy.cpp
    src/data/SqliteDatabase.cpp
    src/data/SqliteBootstrapRepository.cpp
    src/data/SqliteBootstrapDatabaseInitializer.cpp
    src/data/SqliteGameStateRepository.cpp
    src/data/SqliteLeagueRulesRepository.cpp
    src/data/SqliteSaveMetadataRepository.cpp
    src/data/WorldBootstrapLoader.cpp
    src/data/WorldBootstrapService.cpp
    src/competition/SeasonPlan.cpp
    src/roster/Team.cpp
    src/match/EditableLineup.cpp
    src/match/TeamSelectionService.cpp
    src/match/TeamSheet.cpp
    src/interaction/TransferOfferDecisionInteraction.cpp
    src/transfer/TransferOffer.cpp
    src/transfer/TransferOfferService.cpp
    src/transfer/TransferRoom.cpp
    src/core/User.cpp
)

add_executable(fm_runner
    apps/fm_runner/main.cpp
)

add_executable(fm_sqlite_bootstrap_smoke
    apps/sqlite_bootstrap_smoke.cpp
)

add_executable(fm_match_engine_smoke
    apps/match_engine_smoke.cpp
)

add_library(sqlite3 STATIC
    third_party/sqlite/sqlite3.c
)

target_include_directories(sqlite3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)

target_compile_definitions(sqlite3 PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_OMIT_LOAD_EXTENSION
)

if(MSVC)
    target_compile_definitions(sqlite3 PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(fm_core PUBLIC sqlite3)

target_link_libraries(fm_runner PRIVATE fm_core)
target_link_libraries(fm_sqlite_bootstrap_smoke PRIVATE fm_core)
target_link_libraries(fm_match_engine_smoke PRIVATE fm_core)

target_compile_features(fm_core PUBLIC cxx_std_17)
target_compile_features(fm_runner PRIVATE cxx_std_17)
target_compile_features(fm_sqlite_bootstrap_smoke PRIVATE cxx_std_17)
target_compile_features(fm_match_engine_smoke PRIVATE cxx_std_17)

target_include_directories(fm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

function(copy_fm_database_assets target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/database"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/database/schema.sql"
            "$<TARGET_FILE_DIR:${target_name}>/database/schema.sql"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/database/seed.sql"
            "$<TARGET_FILE_DIR:${target_name}>/database/seed.sql"
    )
endfunction()

copy_fm_database_assets(fm_runner)
copy_fm_database_assets(fm_sqlite_bootstrap_smoke)
