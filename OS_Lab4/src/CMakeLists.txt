cmake_minimum_required(VERSION 3.16)

project(OS_Lab4 C)

if(NOT WIN32)
    message(FATAL_ERROR "OS_Lab4 uses the Windows API.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(parent parent.c protocol.h)
add_executable(child child.c protocol.h)
