cmake_minimum_required(VERSION 3.16)
project(FootballManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Add the subdirectories first
add_subdirectory(FM)
add_subdirectory(FM_UI)

# 2. Now that the target FM_UI exists, you can modify it
set_target_properties(FM_UI PROPERTIES OUTPUT_NAME "football_manager_ui")