cmake_minimum_required(VERSION 3.30)
project(ProjetReseaux C)

set(CMAKE_C_STANDARD 11)

add_executable(ProjetReseaux main.c
        include/config.h
        include/server.h
        include/protocol.h
        include/network_core.h
        src/network_core.c
        include/sqlite_utils.h
        src/sqlite_utils.c)

find_package(SQLite3 REQUIRED)

target_link_libraries(ProjetReseaux PRIVATE SQLite::SQLite3)