# Test configuration for ndmonlib

# Test utilities
add_library(test_utils STATIC
    unit/test_utils.c
)
target_include_directories(test_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_utils PRIVATE mon)

# Unit tests
add_executable(test_dispatcher
    unit/test_dispatcher.c
)
target_link_libraries(test_dispatcher PRIVATE mon test_utils)
add_test(NAME dispatcher COMMAND test_dispatcher)

add_executable(test_params
    unit/test_params.c
)
target_link_libraries(test_params PRIVATE mon test_utils)
add_test(NAME params COMMAND test_params)

add_executable(test_file_table
    unit/test_file_table.c
)
target_link_libraries(test_file_table PRIVATE mon test_utils)
add_test(NAME file_table COMMAND test_file_table)

# Integration tests
add_executable(test_mon_handlers
    integration/test_mon_handlers.c
)
target_link_libraries(test_mon_handlers PRIVATE mon test_utils)
add_test(NAME mon_handlers COMMAND test_mon_handlers)

message(STATUS "Test suite configured: ${CMAKE_CTEST_COMMAND} to run")
