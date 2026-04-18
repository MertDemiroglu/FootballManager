cmake_minimum_required(VERSION 3.16)

project(FootballManager LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/msvc2022_64")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(FM)
add_subdirectory(FM_UI)