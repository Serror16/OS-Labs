cmake_minimum_required(VERSION 3.16)

project(OS_Lab2 LANGUAGES C)

if(NOT WIN32)
    message(FATAL_ERROR "OS_Lab2 uses the Windows API and must be built on Windows.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(parent parent.c)
add_executable(child child.c)
