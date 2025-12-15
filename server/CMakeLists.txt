cmake_minimum_required(VERSION 3.30)
project(ProjetReseaux C)

set(CMAKE_C_STANDARD 11)

add_executable(ProjetReseaux
        src/main.c
        src/network_core.c
        src/sqlite_utils.c
        src/auth.c)

target_include_directories(ProjetReseaux PRIVATE include)

find_package(SQLite3 REQUIRED)

target_link_libraries(ProjetReseaux PRIVATE SQLite::SQLite3)