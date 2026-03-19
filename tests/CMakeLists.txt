# MicropolisCore Test Harness
# Includes Golden Master regression testing and Google Test unit tests

# ==============================================================================
# Google Test Setup (fetched automatically)
# ==============================================================================
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ==============================================================================
# Golden Master Test (Regression Testing)
# ==============================================================================
add_executable(golden_master
    golden_master.cpp
)

target_link_libraries(golden_master PRIVATE MicropolisEngine)

add_test(
    NAME GoldenMasterTest
    COMMAND golden_master
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(GoldenMasterTest PROPERTIES
    TIMEOUT 60
    LABELS "regression"
)

# Custom target to generate new golden master data
add_custom_target(generate_golden_master
    COMMAND golden_master --generate
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating new golden master simulation data"
)

# ==============================================================================
# Unit Tests (Google Test)
# ==============================================================================
add_executable(unit_tests
    unit_tests.cpp
    test_world_dimensions.cpp
    test_power_system.cpp
    test_traffic_system.cpp
    test_budget_system.cpp
    test_save_format.cpp
    test_game_config.cpp
    test_scenario_config.cpp
)

target_link_libraries(unit_tests PRIVATE
    MicropolisEngine
    GTest::gtest
    GTest::gtest_main
)

# Enable Google Test discovery
include(GoogleTest)
gtest_discover_tests(unit_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES LABELS "unit"
)

# ==============================================================================
# Integration Tests
# ==============================================================================
add_executable(integration_tests
    integration_tests.cpp
)

target_link_libraries(integration_tests PRIVATE
    MicropolisEngine
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(integration_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES LABELS "integration"
)

# ==============================================================================
# Test Data Directory
# ==============================================================================
# Copy test data to build directory if needed
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/golden_master.dat)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/golden_master.dat
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
