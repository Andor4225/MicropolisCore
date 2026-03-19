/**
 * @file AStarPathfinder.h
 * @brief A* pathfinding algorithm for traffic simulation.
 *
 * MODERNIZATION (Phase 4):
 * Replaces the original random-walk traffic algorithm with goal-directed
 * A* pathfinding. This provides:
 * - Optimal shortest paths to destinations
 * - Optional traffic density weighting for congestion avoidance
 * - Configurable maximum search distance
 */

#ifndef MICROPOLIS_ASTAR_PATHFINDER_H
#define MICROPOLIS_ASTAR_PATHFINDER_H

#include "IMapAccess.h"
#include "../position.h"
#include "../data_types.h"
#include <vector>
#include <queue>
#include <functional>
#include <optional>

namespace MicropolisEngine {

/**
 * @brief Configuration for A* pathfinding behavior.
 */
struct PathfinderConfig {
    int maxSearchDistance = 30;          ///< Maximum tiles to search (default: 30)
    float trafficCostWeight = 0.0f;      ///< Weight for traffic density in cost (0.0 = ignore)
    bool allowDiagonal = false;          ///< Allow diagonal movement (not used in classic)
    float randomizationFactor = 0.0f;    ///< Add randomness to costs (0.0 = deterministic)
};

/**
 * @brief Result of a pathfinding operation.
 */
struct PathResult {
    bool found = false;                  ///< Was a path found?
    std::vector<Position> path;          ///< Path from start to goal (inclusive)
    int nodesExplored = 0;               ///< Number of nodes explored during search
    int pathLength = 0;                  ///< Length of the path found
    Position destination;                ///< Final destination position
};

/**
 * @brief A* pathfinding implementation for traffic simulation.
 *
 * This class implements the A* algorithm optimized for the Micropolis
 * grid-based map. It supports:
 * - Manhattan distance heuristic for orthogonal movement
 * - Optional traffic density weighting
 * - Configurable search limits
 * - Path reconstruction
 *
 * Usage:
 * @code
 *   AStarPathfinder pathfinder(mapAccess);
 *   pathfinder.setConfig({.maxSearchDistance = 50, .trafficCostWeight = 0.5f});
 *
 *   auto result = pathfinder.findPath(start, [&](int x, int y) {
 *       return isDestinationZone(x, y);
 *   });
 *
 *   if (result.found) {
 *       for (const auto& pos : result.path) {
 *           // Use path positions
 *       }
 *   }
 * @endcode
 */
class AStarPathfinder {
public:
    /** Function type for checking if a position is a valid destination. */
    using GoalPredicate = std::function<bool(int x, int y)>;

    /** Random number generator function type. */
    using RngFunction = std::function<int()>;

    /**
     * @brief Construct pathfinder with map access.
     * @param mapAccess Interface for map read operations.
     */
    explicit AStarPathfinder(const IMapReader& mapAccess);

    /**
     * @brief Set pathfinding configuration.
     * @param config New configuration settings.
     */
    void setConfig(const PathfinderConfig& config) { config_ = config; }

    /**
     * @brief Get current configuration.
     * @return Current pathfinding configuration.
     */
    const PathfinderConfig& getConfig() const { return config_; }

    /**
     * @brief Set random number generator for cost randomization.
     * @param rng Random number generator function.
     */
    void setRng(RngFunction rng) { rng_ = std::move(rng); }

    /**
     * @brief Find a path from start to any position matching the goal predicate.
     *
     * @param start Starting position (must be on a road).
     * @param isGoal Predicate that returns true for valid destination tiles.
     * @return PathResult containing the path if found.
     */
    PathResult findPath(const Position& start, GoalPredicate isGoal);

    /**
     * @brief Find a path from start to a specific destination.
     *
     * @param start Starting position.
     * @param goal Exact goal position.
     * @return PathResult containing the path if found.
     */
    PathResult findPathTo(const Position& start, const Position& goal);

    /**
     * @brief Check if a tile can be traversed (is a road).
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return True if the tile is drivable.
     */
    bool isTraversable(int x, int y) const;

private:
    /** Node structure for A* open/closed lists. */
    struct Node {
        Position pos;
        float gCost = 0.0f;      ///< Cost from start to this node
        float hCost = 0.0f;      ///< Heuristic estimate to goal
        float fCost() const { return gCost + hCost; }
        int parentX = -1;
        int parentY = -1;

        // For priority queue (lower f-cost = higher priority)
        bool operator>(const Node& other) const {
            return fCost() > other.fCost();
        }
    };

    /**
     * @brief Calculate heuristic (Manhattan distance).
     * @param from Current position.
     * @param to Target position.
     * @return Estimated cost to reach target.
     */
    float heuristic(const Position& from, const Position& to) const;

    /**
     * @brief Calculate movement cost between adjacent tiles.
     * @param from Source position.
     * @param to Destination position.
     * @return Movement cost (base 1.0, increased by traffic).
     */
    float movementCost(const Position& from, const Position& to) const;

    /**
     * @brief Get valid neighboring positions.
     * @param pos Current position.
     * @return Vector of traversable neighbor positions.
     */
    std::vector<Position> getNeighbors(const Position& pos) const;

    /**
     * @brief Reconstruct path from closed set.
     * @param endNode Final node reached.
     * @param cameFrom Map of positions to their parents.
     * @return Path from start to end.
     */
    std::vector<Position> reconstructPath(
        const Position& end,
        const std::vector<std::vector<Position>>& cameFrom) const;

    /**
     * @brief Test if a tile value represents a road.
     * @param tile Tile value from map.
     * @return True if tile is drivable.
     */
    bool roadTest(MapTile tile) const;

    const IMapReader& mapAccess_;
    PathfinderConfig config_;
    RngFunction rng_;

    // Tile constants (same as original)
    static constexpr MapTile ROADBASE  = 64;
    static constexpr MapTile POWERBASE = 208;
    static constexpr MapTile LASTPOWER = 222;
    static constexpr MapTile LASTRAIL  = 238;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_ASTAR_PATHFINDER_H
