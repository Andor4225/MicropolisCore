# MicropolisEngine - Library CMake Configuration
# Builds the Micropolis game engine as a static/shared library

# Collect all source files (excluding emscripten.cpp which is WebAssembly-specific)
set(MICROPOLIS_SOURCES
    src/allocate.cpp
    src/animate.cpp
    src/budget.cpp
    src/callback.cpp
    src/connect.cpp
    src/disasters.cpp
    src/evaluate.cpp
    src/fileio.cpp
    src/frontendmessage.cpp
    src/generate.cpp
    src/graph.cpp
    src/initialize.cpp
    src/map.cpp
    src/message.cpp
    src/micropolis.cpp
    src/position.cpp
    src/power.cpp
    src/random.cpp
    src/scan.cpp
    src/simulate.cpp
    src/sprite.cpp
    src/tool.cpp
    src/traffic.cpp
    src/update.cpp
    src/utilities.cpp
    src/zone.cpp
    # Phase 3: Extracted subsystems
    src/subsystems/PowerSystem.cpp
    src/subsystems/TrafficSystem.cpp
    src/subsystems/BudgetSystem.cpp
    src/subsystems/DisasterSystem.cpp
    src/subsystems/EvaluationSystem.cpp
    # Phase 4: A* Pathfinding
    src/subsystems/AStarPathfinder.cpp
    # Phase 4: Configuration System
    src/config/GameConfig.cpp
    # Phase 4: World Dimensions
    src/world_dimensions.cpp
    # Phase 5: Modern Save Format
    src/save_format.cpp
    # Phase 6: Async Scanning
    src/async_scanner.cpp
    # Phase 6: Scenario System
    src/config/ScenarioConfig.cpp
)

# Header files (for IDE visibility)
set(MICROPOLIS_HEADERS
    src/callback.h
    src/data_types.h
    src/frontendmessage.h
    src/map_type.h
    src/micropolis.h
    src/platform.h
    src/position.h
    src/text.h
    src/tool.h
    # Phase 3: Events infrastructure
    src/events/EventBus.h
    src/events/EventTypes.h
    # Phase 3: Subsystem interfaces and implementations
    src/subsystems/ISubsystem.h
    src/subsystems/IMapAccess.h
    src/subsystems/PowerSystem.h
    src/subsystems/TrafficSystem.h
    src/subsystems/BudgetSystem.h
    src/subsystems/DisasterSystem.h
    src/subsystems/EvaluationSystem.h
    # Phase 4: A* Pathfinding
    src/subsystems/AStarPathfinder.h
    # Phase 4: Configuration System
    src/config/GameConfig.h
    # Phase 4: World Dimensions
    src/world_dimensions.h
    # Phase 5: Modern Save Format
    src/save_format.h
    # Phase 6: Strong Types
    src/tile_flags.h
    # Phase 6: Async Scanning
    src/async_scanner.h
    # Phase 6: Scenario System
    src/config/ScenarioConfig.h
)

# Create the library target
add_library(MicropolisEngine ${MICROPOLIS_SOURCES} ${MICROPOLIS_HEADERS})

# Add an alias for consistent usage
add_library(Micropolis::Engine ALIAS MicropolisEngine)

# Set target properties
set_target_properties(MicropolisEngine PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "micropolis"
    DEBUG_POSTFIX "d"
    POSITION_INDEPENDENT_CODE ON
)

# Include directories
target_include_directories(MicropolisEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/micropolis>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile definitions
target_compile_definitions(MicropolisEngine
    PRIVATE
        # Define for native builds (vs Emscripten)
        MICROPOLIS_NATIVE_BUILD
        $<$<BOOL:${MICROPOLIS_ENABLE_TELEMETRY}>:MICROPOLIS_TELEMETRY_ENABLED>
)

# Platform-specific link libraries
if(WIN32)
    # Windows doesn't need additional libs for basic functionality
elseif(UNIX AND NOT APPLE)
    # Linux may need math library
    target_link_libraries(MicropolisEngine PRIVATE m)
elseif(APPLE)
    # macOS - no additional libs needed
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(MicropolisEngine PRIVATE
        /wd4100  # unreferenced formal parameter (common in callbacks)
        /wd4244  # conversion from 'type1' to 'type2' (legacy code has many)
        /wd4267  # conversion from 'size_t' to 'type' (will fix in Phase 2)
    )
else()
    target_compile_options(MicropolisEngine PRIVATE
        -Wno-unused-parameter  # Many callback parameters unused
        -Wno-sign-compare      # Legacy code comparisons
    )
endif()

# Source groups for IDE
source_group("Source Files" FILES ${MICROPOLIS_SOURCES})
source_group("Header Files" FILES ${MICROPOLIS_HEADERS})
source_group("Events" FILES
    src/events/EventBus.h
    src/events/EventTypes.h
)
source_group("Subsystems" FILES
    src/subsystems/ISubsystem.h
    src/subsystems/IMapAccess.h
    src/subsystems/PowerSystem.h
    src/subsystems/PowerSystem.cpp
    src/subsystems/TrafficSystem.h
    src/subsystems/TrafficSystem.cpp
    src/subsystems/BudgetSystem.h
    src/subsystems/BudgetSystem.cpp
    src/subsystems/DisasterSystem.h
    src/subsystems/DisasterSystem.cpp
    src/subsystems/EvaluationSystem.h
    src/subsystems/EvaluationSystem.cpp
    src/subsystems/AStarPathfinder.h
    src/subsystems/AStarPathfinder.cpp
)
source_group("Config" FILES
    src/config/GameConfig.h
    src/config/GameConfig.cpp
)

# Export symbols for shared library builds
include(GenerateExportHeader)
generate_export_header(MicropolisEngine
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/micropolis_export.h
    EXPORT_MACRO_NAME MICROPOLIS_API
)

target_include_directories(MicropolisEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
