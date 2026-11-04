cmake_minimum_required(VERSION 3.16)

project(OS_Lab3 LANGUAGES C)

if(NOT WIN32)
    message(FATAL_ERROR "OS_Lab3 uses the Windows thread API.")
endif()

add_executable(os_lab3
    main.c
    options.c
    points.c
    kmeans.c
)
set_target_properties(os_lab3 PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

