/**
 * @file AStarPathfinder.cpp
 * @brief A* pathfinding implementation for traffic simulation.
 *
 * MODERNIZATION (Phase 4):
 * Implements the A* algorithm for goal-directed traffic pathfinding.
 */

#include "AStarPathfinder.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace MicropolisEngine {

AStarPathfinder::AStarPathfinder(const IMapReader& mapAccess)
    : mapAccess_(mapAccess)
    , config_()
    , rng_(nullptr)
{
}

PathResult AStarPathfinder::findPath(const Position& start, GoalPredicate isGoal)
{
    PathResult result;
    result.found = false;
    result.nodesExplored = 0;

    const int worldW = mapAccess_.getWorldWidth();
    const int worldH = mapAccess_.getWorldHeight();

    // Check start is valid
    if (!mapAccess_.isOnMap(start.posX, start.posY)) {
        return result;
    }

    // Allocate tracking arrays
    std::vector<std::vector<float>> gCosts(worldW,
        std::vector<float>(worldH, std::numeric_limits<float>::infinity()));
    std::vector<std::vector<bool>> closed(worldW,
        std::vector<bool>(worldH, false));
    std::vector<std::vector<Position>> cameFrom(worldW,
        std::vector<Position>(worldH, Position(-1, -1)));

    // Priority queue (min-heap by f-cost)
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;

    // Initialize start node
    Node startNode;
    startNode.pos = start;
    startNode.gCost = 0.0f;
    startNode.hCost = 0.0f;  // No specific goal position for heuristic
    startNode.parentX = -1;
    startNode.parentY = -1;

    openSet.push(startNode);
    gCosts[start.posX][start.posY] = 0.0f;

    // Search limit based on max distance
    const int maxNodes = config_.maxSearchDistance * config_.maxSearchDistance * 4;

    while (!openSet.empty() && result.nodesExplored < maxNodes) {
        Node current = openSet.top();
        openSet.pop();

        int cx = current.pos.posX;
        int cy = current.pos.posY;

        // Skip if already processed
        if (closed[cx][cy]) {
            continue;
        }
        closed[cx][cy] = true;
        result.nodesExplored++;

        // Check if this position reaches a goal
        // Look at adjacent tiles for destination zones
        bool foundGoal = false;
        Position goalPos;

        // Check all 4 directions for goal zones
        const int dx[] = {0, 1, 0, -1};
        const int dy[] = {-1, 0, 1, 0};

        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];

            if (mapAccess_.isOnMap(nx, ny) && isGoal(nx, ny)) {
                foundGoal = true;
                goalPos = Position(nx, ny);
                break;
            }
        }

        if (foundGoal) {
            // Reconstruct path
            result.found = true;
            result.destination = goalPos;
            result.path = reconstructPath(current.pos, cameFrom);
            result.pathLength = static_cast<int>(result.path.size());
            return result;
        }

        // Check if we've exceeded max travel distance
        if (current.gCost > static_cast<float>(config_.maxSearchDistance)) {
            continue;  // Don't expand this node further
        }

        // Expand neighbors
        std::vector<Position> neighbors = getNeighbors(current.pos);

        for (const Position& neighbor : neighbors) {
            int nx = neighbor.posX;
            int ny = neighbor.posY;

            if (closed[nx][ny]) {
                continue;
            }

            float tentativeG = current.gCost + movementCost(current.pos, neighbor);

            if (tentativeG < gCosts[nx][ny]) {
                // This path is better
                gCosts[nx][ny] = tentativeG;
                cameFrom[nx][ny] = current.pos;

                Node neighborNode;
                neighborNode.pos = neighbor;
                neighborNode.gCost = tentativeG;
                neighborNode.hCost = 0.0f;  // Dijkstra-style (no target position known)
                neighborNode.parentX = cx;
                neighborNode.parentY = cy;

                openSet.push(neighborNode);
            }
        }
    }

    // No path found
    return result;
}

PathResult AStarPathfinder::findPathTo(const Position& start, const Position& goal)
{
    PathResult result;
    result.found = false;
    result.nodesExplored = 0;
    result.destination = goal;

    const int worldW = mapAccess_.getWorldWidth();
    const int worldH = mapAccess_.getWorldHeight();

    // Validate positions
    if (!mapAccess_.isOnMap(start.posX, start.posY) ||
        !mapAccess_.isOnMap(goal.posX, goal.posY)) {
        return result;
    }

    // Allocate tracking arrays
    std::vector<std::vector<float>> gCosts(worldW,
        std::vector<float>(worldH, std::numeric_limits<float>::infinity()));
    std::vector<std::vector<bool>> closed(worldW,
        std::vector<bool>(worldH, false));
    std::vector<std::vector<Position>> cameFrom(worldW,
        std::vector<Position>(worldH, Position(-1, -1)));

    // Priority queue
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;

    // Initialize start node
    Node startNode;
    startNode.pos = start;
    startNode.gCost = 0.0f;
    startNode.hCost = heuristic(start, goal);
    startNode.parentX = -1;
    startNode.parentY = -1;

    openSet.push(startNode);
    gCosts[start.posX][start.posY] = 0.0f;

    const int maxNodes = config_.maxSearchDistance * config_.maxSearchDistance * 4;

    while (!openSet.empty() && result.nodesExplored < maxNodes) {
        Node current = openSet.top();
        openSet.pop();

        int cx = current.pos.posX;
        int cy = current.pos.posY;

        if (closed[cx][cy]) {
            continue;
        }
        closed[cx][cy] = true;
        result.nodesExplored++;

        // Check if we reached the goal
        if (cx == goal.posX && cy == goal.posY) {
            result.found = true;
            result.path = reconstructPath(current.pos, cameFrom);
            result.pathLength = static_cast<int>(result.path.size());
            return result;
        }

        // Check max distance
        if (current.gCost > static_cast<float>(config_.maxSearchDistance)) {
            continue;
        }

        // Expand neighbors
        std::vector<Position> neighbors = getNeighbors(current.pos);

        for (const Position& neighbor : neighbors) {
            int nx = neighbor.posX;
            int ny = neighbor.posY;

            if (closed[nx][ny]) {
                continue;
            }

            float tentativeG = current.gCost + movementCost(current.pos, neighbor);

            if (tentativeG < gCosts[nx][ny]) {
                gCosts[nx][ny] = tentativeG;
                cameFrom[nx][ny] = current.pos;

                Node neighborNode;
                neighborNode.pos = neighbor;
                neighborNode.gCost = tentativeG;
                neighborNode.hCost = heuristic(neighbor, goal);
                neighborNode.parentX = cx;
                neighborNode.parentY = cy;

                openSet.push(neighborNode);
            }
        }
    }

    return result;
}

bool AStarPathfinder::isTraversable(int x, int y) const
{
    if (!mapAccess_.isOnMap(x, y)) {
        return false;
    }
    return roadTest(mapAccess_.getTile(x, y));
}

float AStarPathfinder::heuristic(const Position& from, const Position& to) const
{
    // Manhattan distance for orthogonal movement
    return static_cast<float>(std::abs(from.posX - to.posX) +
                              std::abs(from.posY - to.posY));
}

float AStarPathfinder::movementCost(const Position& from, const Position& to) const
{
    float cost = 1.0f;

    // Add traffic density cost if configured
    if (config_.trafficCostWeight > 0.0f) {
        int traffic = mapAccess_.getTrafficDensity(to.posX, to.posY);
        // Traffic density is 0-240, normalize to 0-1 and apply weight
        float trafficPenalty = (static_cast<float>(traffic) / 240.0f) * config_.trafficCostWeight;
        cost += trafficPenalty;
    }

    // Add randomization if configured
    if (config_.randomizationFactor > 0.0f && rng_) {
        // Add random factor between 0 and randomizationFactor
        float randomValue = static_cast<float>(rng_() & 0xFF) / 255.0f;
        cost += randomValue * config_.randomizationFactor;
    }

    return cost;
}

std::vector<Position> AStarPathfinder::getNeighbors(const Position& pos) const
{
    std::vector<Position> neighbors;
    neighbors.reserve(4);

    // Four cardinal directions
    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    for (int d = 0; d < 4; d++) {
        int nx = pos.posX + dx[d];
        int ny = pos.posY + dy[d];

        if (isTraversable(nx, ny)) {
            neighbors.emplace_back(nx, ny);
        }
    }

    return neighbors;
}

std::vector<Position> AStarPathfinder::reconstructPath(
    const Position& end,
    const std::vector<std::vector<Position>>& cameFrom) const
{
    std::vector<Position> path;
    Position current = end;

    while (current.posX >= 0 && current.posY >= 0) {
        path.push_back(current);
        Position parent = cameFrom[current.posX][current.posY];
        if (parent.posX < 0 || parent.posY < 0) {
            break;
        }
        current = parent;
    }

    // Reverse to get start-to-end order
    std::reverse(path.begin(), path.end());
    return path;
}

bool AStarPathfinder::roadTest(MapTile tile) const
{
    tile = tile & TileFlags::LOMASK;

    if (tile < ROADBASE || tile > LASTRAIL) {
        return false;
    }

    if (tile >= POWERBASE && tile < LASTPOWER) {
        return false;
    }

    return true;
}

} // namespace MicropolisEngine
