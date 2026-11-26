cmake_minimum_required(VERSION 3.16)

project(OS_Lab5 LANGUAGES C)

if(NOT WIN32)
    message(FATAL_ERROR "OS_Lab5 requires Windows for DLL loading.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(MINGW)
    # This toolchain cannot generate an import library during DLL linking.
    # MODULE builds the DLL; dlltool creates the import library separately.
    add_library(lab5_impl1 MODULE impl1.c)
    add_library(lab5_impl2 MODULE impl2.c)
else()
    add_library(lab5_impl1 SHARED impl1.c)
    add_library(lab5_impl2 SHARED impl2.c)
endif()

foreach(library_target IN ITEMS lab5_impl1 lab5_impl2)
    target_include_directories(${library_target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(${library_target} PRIVATE LAB5_BUILD_DLL)
    set_target_properties(${library_target} PROPERTIES PREFIX "")
endforeach()

add_executable(lab5_linked linked_main.c commands.c)
target_include_directories(lab5_linked PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
if(MINGW)
    find_program(LAB5_DLLTOOL NAMES dlltool)
    if(NOT LAB5_DLLTOOL)
        message(FATAL_ERROR "MinGW build requires dlltool to create the import library.")
    endif()
    set(LAB5_IMPORT_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/lib/lab5_impl1_import.a")
    add_custom_command(
        OUTPUT "${LAB5_IMPORT_LIBRARY}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/lib"
        COMMAND "${LAB5_DLLTOOL}" --def "${CMAKE_CURRENT_SOURCE_DIR}/lab5_impl1.def"
                --dllname lab5_impl1.dll --output-lib "${LAB5_IMPORT_LIBRARY}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lab5_impl1.def"
        VERBATIM
    )
    add_custom_target(lab5_import DEPENDS "${LAB5_IMPORT_LIBRARY}")
    add_dependencies(lab5_linked lab5_impl1 lab5_import)
    target_link_libraries(lab5_linked PRIVATE "${LAB5_IMPORT_LIBRARY}")
else()
    target_link_libraries(lab5_linked PRIVATE lab5_impl1)
endif()

add_executable(lab5_runtime runtime_main.c commands.c)
target_include_directories(lab5_runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # GetProcAddress returns FARPROC, which must be cast to each exported signature.
    target_compile_options(lab5_runtime PRIVATE -Wno-cast-function-type)
endif()

foreach(target IN ITEMS lab5_impl1 lab5_impl2 lab5_linked lab5_runtime)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
endforeach()
