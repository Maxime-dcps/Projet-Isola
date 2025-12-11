cmake_minimum_required(VERSION 3.30)
project(ProjetReseaux C)

set(CMAKE_C_STANDARD 11)

add_executable(ProjetReseaux main.c
        display.c
        display.h
        include/config.h
        include/config.h
        include/server.h
        include/server.h
        include/protocol.h
        include/network_core.h
        network_core.c
        network_core.c)