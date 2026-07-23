cmake_minimum_required(VERSION 3.16)
project(ndmonlib VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Collect handler source files
file(GLOB MON_HANDLER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/handlers/*.c
)

# Create the MON library
add_library(mon STATIC
    src/core/mon_dispatch.c
    src/core/mon_registry.c
    src/core/mon_params.c
    src/core/mon_log.c
    src/support/mon_file_table.c
    src/support/mon_terminal_state.c
    src/support/mon_clock.c
    src/support/mon_config.c
    src/support/mon_path.c
    ${MON_HANDLER_SOURCES}
)

# Include directories - use same approach as nd500x (add all for both target AND callers)
target_include_directories(mon PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ndmon
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile options
target_compile_options(mon PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Count handlers for status message
list(LENGTH MON_HANDLER_SOURCES MON_HANDLER_COUNT)
message(STATUS "ndmonlib: Found ${MON_HANDLER_COUNT} MON handlers")
message(STATUS "ndmonlib: Building as standalone library")

# Enable testing
enable_testing()
add_subdirectory(test)
