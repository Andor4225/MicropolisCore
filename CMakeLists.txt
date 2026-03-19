# MicropolisCore - Root CMake Configuration
# Modernized build system for the Micropolis game engine
#
# Usage:
#   mkdir build && cd build
#   cmake ..
#   cmake --build .

cmake_minimum_required(VERSION 3.16)

project(MicropolisCore
    VERSION 1.0.0
    DESCRIPTION "Open-source Micropolis (SimCity Classic) game engine"
    LANGUAGES CXX
)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(MICROPOLIS_BUILD_TESTS "Build the test harness" ON)
option(MICROPOLIS_ENABLE_TELEMETRY "Enable telemetry/stat streaming" OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(WIN32)
    # Windows-specific definitions
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

    # Enable parallel builds with MSVC
    if(MSVC)
        add_compile_options(/MP)
        # Warning level 4, treat warnings as errors in release
        add_compile_options(/W4)
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            add_compile_options(/WX)
        endif()
    endif()
elseif(UNIX)
    # Unix/Linux/macOS settings
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-Werror)
    endif()
endif()

# Add the engine library
add_subdirectory(MicropolisEngine)

# Test harness
if(MICROPOLIS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS MicropolisEngine
    EXPORT MicropolisEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY MicropolisEngine/src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/micropolis
    FILES_MATCHING PATTERN "*.h"
)

# Export targets for find_package() support
install(EXPORT MicropolisEngineTargets
    FILE MicropolisEngineTargets.cmake
    NAMESPACE Micropolis::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MicropolisEngine
)

# Generate package config
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MicropolisEngineConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MicropolisEngineConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MicropolisEngine
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/MicropolisEngineConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/MicropolisEngineConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/MicropolisEngineConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MicropolisEngine
)

# Print configuration summary
message(STATUS "")
message(STATUS "MicropolisCore Configuration:")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "  Shared libs:      ${BUILD_SHARED_LIBS}")
message(STATUS "  Build tests:      ${MICROPOLIS_BUILD_TESTS}")
message(STATUS "  Telemetry:        ${MICROPOLIS_ENABLE_TELEMETRY}")
message(STATUS "")
