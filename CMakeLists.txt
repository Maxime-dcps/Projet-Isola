cmake_minimum_required(VERSION 3.30)
project(ProjetReseaux C)

set(CMAKE_C_STANDARD 11)

add_executable(ProjetReseaux main.c
        display.c
        display.h)