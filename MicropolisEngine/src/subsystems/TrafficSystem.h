/**
 * @file TrafficSystem.h
 * @brief Traffic simulation subsystem.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from traffic.cpp to provide a standalone, testable traffic
 * simulation system. The TrafficSystem handles:
 * - Traffic pathfinding from zones to destinations
 * - Traffic density map updates
 * - Traffic hotspot tracking
 *
 * MODERNIZATION (Phase 4):
 * Added A* pathfinding as an alternative to the original random walk algorithm.
 * The algorithm can be selected via TrafficConfig.
 */

#ifndef MICROPOLIS_TRAFFIC_SYSTEM_H
#define MICROPOLIS_TRAFFIC_SYSTEM_H

#include "ISubsystem.h"
#include "IMapAccess.h"
#include "AStarPathfinder.h"
#include "../events/EventBus.h"
#include "../events/EventTypes.h"
#include "../position.h"
#include "../data_types.h"
#include <functional>
#include <memory>

// Forward declaration - ZoneType is defined in micropolis.h
enum ZoneType;

namespace MicropolisEngine {

/** Default maximum distance to search for traffic destination. */
constexpr int DEFAULT_MAX_TRAFFIC_DISTANCE = 30;

/** Legacy constant for backwards compatibility. */
constexpr int MAX_TRAFFIC_DISTANCE = DEFAULT_MAX_TRAFFIC_DISTANCE;

/**
 * @brief Pathfinding algorithm selection.
 */
enum class PathfindingAlgorithm {
    RandomWalk,   ///< Original random walk with backtracking (classic behavior)
    AStar         ///< A* shortest path (modern, more efficient)
};

/**
 * @brief Configuration for TrafficSystem behavior.
 */
struct TrafficConfig {
    PathfindingAlgorithm algorithm = PathfindingAlgorithm::RandomWalk;
    int maxSearchDistance = DEFAULT_MAX_TRAFFIC_DISTANCE;
    float trafficCostWeight = 0.0f;     ///< A* only: weight for traffic density
    float randomizationFactor = 0.0f;   ///< A* only: add randomness to paths
};

/**
 * @brief Traffic simulation subsystem.
 *
 * Handles traffic simulation including pathfinding from zones to
 * destinations and traffic density updates. Supports two algorithms:
 * - RandomWalk: Original algorithm with random choices and backtracking
 * - AStar: Modern A* pathfinding for optimal routes
 *
 * Usage:
 * @code
 *   TrafficSystem traffic(mapAccess, eventBus,
 *                         [this]() { return getRandom16(); });
 *   traffic.initialize();
 *
 *   // Optional: Configure for A* pathfinding
 *   TrafficConfig config;
 *   config.algorithm = PathfindingAlgorithm::AStar;
 *   config.maxSearchDistance = 50;
 *   traffic.setConfig(config);
 *
 *   // Attempt to find traffic route
 *   short result = traffic.makeTraffic(pos, ZoneType::Commercial);
 *   // result: 1 = found, 0 = no route, -1 = no road
 *
 *   // Get current traffic hotspot
 *   Position hotspot = traffic.getTrafficHotspot();
 * @endcode
 */
class TrafficSystem : public ITickableSubsystem {
public:
    /** Random number generator function type (returns 16-bit value). */
    using RngFunction = std::function<int()>;

    /**
     * @brief Construct a TrafficSystem with injected dependencies.
     * @param mapAccess Interface for map read/write operations.
     * @param eventBus Event bus for publishing traffic events.
     * @param rng Random number generator function.
     */
    TrafficSystem(IMapAccess& mapAccess, EventBus& eventBus, RngFunction rng);

    ~TrafficSystem() override = default;

    // ISubsystem interface
    const char* getName() const override { return "TrafficSystem"; }
    void initialize() override;
    void reset() override;
    bool isInitialized() const override { return initialized_; }

    // ITickableSubsystem interface
    void tick(int simCycle, int phase) override;

    /**
     * @brief Set traffic system configuration.
     * @param config New configuration settings.
     */
    void setConfig(const TrafficConfig& config);

    /**
     * @brief Get current configuration.
     * @return Current traffic configuration.
     */
    const TrafficConfig& getConfig() const { return config_; }

    /**
     * @brief Find a connection over a road from a position to a zone type.
     *
     * Searches for a road on the zone perimeter, then attempts to drive
     * to the destination zone type.
     *
     * @param startPos Start position (zone center).
     * @param dest Zone type to find.
     * @return 1 if connection found, 0 if no route, -1 if no road on perimeter.
     */
    short makeTraffic(const Position& startPos, ZoneType dest);

    /**
     * @brief Find a connection directly from a road tile.
     *
     * Unlike makeTraffic(), this assumes the starting position is already
     * on a road tile.
     *
     * @param x Start x position.
     * @param y Start y position.
     * @param dest Zone type to find.
     * @return 1 if connection found, 0 if not found.
     */
    short makeTrafficAt(int x, int y, ZoneType dest);

    /**
     * @brief Get the current traffic hotspot position.
     * @return Position of the most recent heavy traffic location.
     */
    Position getTrafficHotspot() const { return Position(trafMaxX_, trafMaxY_); }

    /**
     * @brief Get stack depth for debugging (random walk only).
     * @return Current position stack depth.
     */
    int getStackDepth() const { return curMapStackPointer_; }

    /**
     * @brief Get path from last A* search (A* only).
     * @return Vector of positions in the last found path.
     */
    const std::vector<Position>& getLastPath() const { return lastAStarPath_; }

private:
    /**
     * @brief Find a road connection on the zone perimeter.
     * @param pos Starting position, updated to road position if found.
     * @return True if a road was found on the perimeter.
     */
    bool findPerimeterRoad(Position* pos);

    /**
     * @brief Try to drive from a position to a destination zone type.
     * @param startPos Starting road position.
     * @param destZone Zone type to find.
     * @return True if destination reached within MAX_TRAFFIC_DISTANCE.
     */
    bool tryDrive(const Position& startPos, ZoneType destZone);

    /**
     * @brief Try to move one step toward destination.
     * @param pos Current position.
     * @param dirLast Last direction moved (to prevent backtracking).
     * @return Direction to move, or DIR2_INVALID if dead end.
     */
    Direction2 tryGo(const Position& pos, Direction2 dirLast);

    /**
     * @brief Check if destination has been reached.
     * @param pos Current position.
     * @param destZone Zone type we're looking for.
     * @return True if adjacent to a matching zone.
     */
    bool driveDone(const Position& pos, ZoneType destZone);

    /**
     * @brief Update the traffic density map from the position stack.
     */
    void addToTrafficDensityMap();

    /**
     * @brief Push a position onto the path stack.
     * @param pos Position to push.
     */
    void pushPos(const Position& pos);

    /**
     * @brief Pull a position from the path stack.
     * @return Position from top of stack.
     */
    Position pullPos();

    /**
     * @brief Get neighboring tile in a direction.
     * @param pos Current position.
     * @param dir Direction to look.
     * @param defaultTile Tile to return if off-map.
     * @return Tile value in that direction.
     */
    MapTile getTileFromMap(const Position& pos, Direction2 dir, MapTile defaultTile);

    /**
     * @brief Test if a tile can be used as a road.
     * @param tile Tile value from map.
     * @return True if tile is drivable.
     */
    bool roadTest(MapTile tile);

    // A* pathfinding methods
    /**
     * @brief Try to find route using A* algorithm.
     * @param startPos Starting road position.
     * @param destZone Zone type to find.
     * @return True if destination reached.
     */
    bool tryDriveAStar(const Position& startPos, ZoneType destZone);

    /**
     * @brief Check if tile matches destination zone type.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param destZone Target zone type.
     * @return True if tile is valid destination.
     */
    bool isDestinationTile(int x, int y, ZoneType destZone) const;

    /**
     * @brief Update traffic density from A* path.
     * @param path Path to update density along.
     */
    void addPathToTrafficDensityMap(const std::vector<Position>& path);

    IMapAccess& mapAccess_;
    EventBus& eventBus_;
    RngFunction rng_;

    // Configuration
    TrafficConfig config_;

    // Random walk state
    int curMapStackPointer_;
    std::vector<Position> curMapStackXY_;  // Dynamic size based on config

    // A* pathfinder
    std::unique_ptr<AStarPathfinder> pathfinder_;
    std::vector<Position> lastAStarPath_;

    // Shared state
    int trafMaxX_;
    int trafMaxY_;

    bool initialized_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_TRAFFIC_SYSTEM_H
