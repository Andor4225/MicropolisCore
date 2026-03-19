/**
 * @file TrafficSystem.cpp
 * @brief Traffic simulation subsystem implementation.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from traffic.cpp. This implementation maintains the original
 * random-walk pathfinding algorithm while providing a cleaner, testable interface.
 *
 * MODERNIZATION (Phase 4):
 * Added A* pathfinding as an alternative algorithm. The algorithm can be
 * selected via TrafficConfig.
 */

#include "TrafficSystem.h"
#include <cassert>
#include <algorithm>

// Tile constants needed for traffic simulation
// These match the values in micropolis.h but are defined locally to avoid
// namespace conflicts with the Micropolis class.
namespace {
    constexpr MapTile DIRT       = 0;
    constexpr MapTile ROADBASE   = 64;   // HBRIDGE
    constexpr MapTile POWERBASE  = 208;  // HPOWER
    constexpr MapTile LASTPOWER  = 222;  // RAILVPOWERH
    constexpr MapTile LASTRAIL   = 238;

    // Destination tile ranges
    constexpr MapTile LHTHR      = 249;  // HOUSE - first single tile house
    constexpr MapTile COMBASE    = 423;  // First commercial tile
    constexpr MapTile PORT       = 698;  // Seaport center
    constexpr MapTile NUCLEAR    = 816;  // Nuclear power plant center
}

namespace MicropolisEngine {

TrafficSystem::TrafficSystem(IMapAccess& mapAccess, EventBus& eventBus, RngFunction rng)
    : mapAccess_(mapAccess)
    , eventBus_(eventBus)
    , rng_(std::move(rng))
    , config_()
    , curMapStackPointer_(0)
    , curMapStackXY_(DEFAULT_MAX_TRAFFIC_DISTANCE + 1)
    , pathfinder_(nullptr)
    , lastAStarPath_()
    , trafMaxX_(0)
    , trafMaxY_(0)
    , initialized_(false)
{
}

void TrafficSystem::initialize()
{
    curMapStackPointer_ = 0;
    trafMaxX_ = 0;
    trafMaxY_ = 0;

    // Resize stack based on config
    curMapStackXY_.resize(config_.maxSearchDistance + 1);

    // Initialize A* pathfinder
    pathfinder_ = std::make_unique<AStarPathfinder>(mapAccess_);
    PathfinderConfig pfConfig;
    pfConfig.maxSearchDistance = config_.maxSearchDistance;
    pfConfig.trafficCostWeight = config_.trafficCostWeight;
    pfConfig.randomizationFactor = config_.randomizationFactor;
    pathfinder_->setConfig(pfConfig);
    pathfinder_->setRng(rng_);

    initialized_ = true;
}

void TrafficSystem::reset()
{
    curMapStackPointer_ = 0;
    lastAStarPath_.clear();
}

void TrafficSystem::setConfig(const TrafficConfig& config)
{
    config_ = config;

    // Update stack size
    curMapStackXY_.resize(config_.maxSearchDistance + 1);

    // Update pathfinder if initialized
    if (pathfinder_) {
        PathfinderConfig pfConfig;
        pfConfig.maxSearchDistance = config_.maxSearchDistance;
        pfConfig.trafficCostWeight = config_.trafficCostWeight;
        pfConfig.randomizationFactor = config_.randomizationFactor;
        pathfinder_->setConfig(pfConfig);
    }
}

void TrafficSystem::tick(int simCycle, int phase)
{
    // Traffic simulation is driven by zone evaluation, not tick-based.
    // This is a no-op for now.
    (void)simCycle;
    (void)phase;
}

short TrafficSystem::makeTraffic(const Position& startPos, ZoneType dest)
{
    curMapStackPointer_ = 0;  // Clear position stack
    lastAStarPath_.clear();

    Position pos(startPos);

    if (findPerimeterRoad(&pos)) {
        bool found = false;

        if (config_.algorithm == PathfindingAlgorithm::AStar) {
            found = tryDriveAStar(pos, dest);
            if (found) {
                addPathToTrafficDensityMap(lastAStarPath_);
            }
        } else {
            // Original random walk algorithm
            found = tryDrive(pos, dest);
            if (found) {
                addToTrafficDensityMap();
            }
        }

        return found ? 1 : 0;
    } else {
        return -1;  // No road found
    }
}

short TrafficSystem::makeTrafficAt(int x, int y, ZoneType dest)
{
    Position pos;
    pos.posX = x;
    pos.posY = y;

    curMapStackPointer_ = 0;
    lastAStarPath_.clear();

    bool found = false;

    if (config_.algorithm == PathfindingAlgorithm::AStar) {
        found = tryDriveAStar(pos, dest);
        if (found) {
            addPathToTrafficDensityMap(lastAStarPath_);
        }
    } else {
        found = tryDrive(pos, dest);
        if (found) {
            addToTrafficDensityMap();
        }
    }

    return found ? 1 : 0;
}

// ============================================================================
// Random Walk Algorithm (Original)
// ============================================================================

void TrafficSystem::addToTrafficDensityMap()
{
    while (curMapStackPointer_ > 0) {
        Position pos = pullPos();

        if (pos.testBounds()) {
            MapTile tile = mapAccess_.getTile(pos.posX, pos.posY) & TileFlags::LOMASK;

            if (tile >= ROADBASE && tile < POWERBASE) {
                // Update traffic density
                int traffic = mapAccess_.getTrafficDensity(pos.posX, pos.posY);
                traffic += 50;
                if (traffic > 240) {
                    traffic = 240;
                }
                mapAccess_.setTrafficDensity(pos.posX, pos.posY, traffic);

                // Check for heavy traffic
                if (traffic >= 240 && (rng_() & 7) == 0) {
                    trafMaxX_ = pos.posX;
                    trafMaxY_ = pos.posY;

                    // Emit heavy traffic event
                    eventBus_.publish(Events::HeavyTraffic{
                        Position(pos.posX, pos.posY),
                        traffic
                    });
                }
            }
        }
    }
}

void TrafficSystem::pushPos(const Position& pos)
{
    curMapStackPointer_++;
    assert(curMapStackPointer_ < static_cast<int>(curMapStackXY_.size()));
    curMapStackXY_[curMapStackPointer_] = pos;
}

Position TrafficSystem::pullPos()
{
    assert(curMapStackPointer_ > 0);
    curMapStackPointer_--;
    return curMapStackXY_[curMapStackPointer_ + 1];
}

bool TrafficSystem::findPerimeterRoad(Position* pos)
{
    // Look for road on edges of zone
    static const short PerimX[12] = {-1, 0, 1, 2, 2, 2, 1, 0,-1,-2,-2,-2};
    static const short PerimY[12] = {-2,-2,-2,-1, 0, 1, 2, 2, 2, 1, 0,-1};

    for (short z = 0; z < 12; z++) {
        short tx = pos->posX + PerimX[z];
        short ty = pos->posY + PerimY[z];

        if (mapAccess_.isOnMap(tx, ty)) {
            MapTile tile = mapAccess_.getTile(tx, ty);
            if (roadTest(tile)) {
                pos->posX = tx;
                pos->posY = ty;
                return true;
            }
        }
    }

    return false;
}

bool TrafficSystem::tryDrive(const Position& startPos, ZoneType destZone)
{
    Direction2 dirLast = DIR2_INVALID;
    Position drivePos(startPos);

    // Maximum distance to try
    for (int dist = 0; dist < config_.maxSearchDistance; dist++) {
        Direction2 dir = tryGo(drivePos, dirLast);

        if (dir != DIR2_INVALID) {
            drivePos.move(dir);
            dirLast = rotate180(dir);

            // Save pos every other move (relates to traffic density map block size)
            if (dist & 1) {
                pushPos(drivePos);
            }

            if (driveDone(drivePos, destZone)) {
                return true;  // Pass
            }
        } else {
            if (curMapStackPointer_ > 0) {
                // Dead end, backup
                curMapStackPointer_--;
                dist += 3;
            } else {
                return false;  // Give up at start
            }
        }
    }

    return false;  // Gone max search distance
}

Direction2 TrafficSystem::tryGo(const Position& pos, Direction2 dirLast)
{
    Direction2 directions[4];

    // Find connections from current position
    Direction2 dir = DIR2_NORTH;
    int count = 0;

    for (int i = 0; i < 4; i++) {
        if (dir != dirLast && roadTest(getTileFromMap(pos, dir, DIRT))) {
            directions[i] = dir;
            count++;
        } else {
            directions[i] = DIR2_INVALID;
        }
        dir = rotate90(dir);
    }

    if (count == 0) {
        return DIR2_INVALID;  // Dead end
    }

    if (count == 1) {
        // Only one solution
        for (int i = 0; i < 4; i++) {
            if (directions[i] != DIR2_INVALID) {
                return directions[i];
            }
        }
    }

    // More than one choice, draw a random number
    int i = rng_() & 3;
    while (directions[i] == DIR2_INVALID) {
        i = (i + 1) & 3;
    }
    return directions[i];
}

MapTile TrafficSystem::getTileFromMap(const Position& pos, Direction2 dir, MapTile defaultTile)
{
    switch (dir) {
        case DIR2_NORTH:
            if (pos.posY > 0) {
                return mapAccess_.getTile(pos.posX, pos.posY - 1) & TileFlags::LOMASK;
            }
            return defaultTile;

        case DIR2_EAST:
            if (pos.posX < mapAccess_.getWorldWidth() - 1) {
                return mapAccess_.getTile(pos.posX + 1, pos.posY) & TileFlags::LOMASK;
            }
            return defaultTile;

        case DIR2_SOUTH:
            if (pos.posY < mapAccess_.getWorldHeight() - 1) {
                return mapAccess_.getTile(pos.posX, pos.posY + 1) & TileFlags::LOMASK;
            }
            return defaultTile;

        case DIR2_WEST:
            if (pos.posX > 0) {
                return mapAccess_.getTile(pos.posX - 1, pos.posY) & TileFlags::LOMASK;
            }
            return defaultTile;

        default:
            return defaultTile;
    }
}

bool TrafficSystem::driveDone(const Position& pos, ZoneType destZone)
{
    // Commercial, industrial, residential destinations
    static const MapTile targetLow[3] = {COMBASE, LHTHR, LHTHR};
    static const MapTile targetHigh[3] = {NUCLEAR, PORT, COMBASE};

    MapTile l = targetLow[destZone];
    MapTile h = targetHigh[destZone];

    // Check all four directions for destination
    if (pos.posY > 0) {
        MapTile z = mapAccess_.getTile(pos.posX, pos.posY - 1) & TileFlags::LOMASK;
        if (z >= l && z <= h) {
            return true;
        }
    }

    if (pos.posX < mapAccess_.getWorldWidth() - 1) {
        MapTile z = mapAccess_.getTile(pos.posX + 1, pos.posY) & TileFlags::LOMASK;
        if (z >= l && z <= h) {
            return true;
        }
    }

    if (pos.posY < mapAccess_.getWorldHeight() - 1) {
        MapTile z = mapAccess_.getTile(pos.posX, pos.posY + 1) & TileFlags::LOMASK;
        if (z >= l && z <= h) {
            return true;
        }
    }

    if (pos.posX > 0) {
        MapTile z = mapAccess_.getTile(pos.posX - 1, pos.posY) & TileFlags::LOMASK;
        if (z >= l && z <= h) {
            return true;
        }
    }

    return false;
}

bool TrafficSystem::roadTest(MapTile tile)
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

// ============================================================================
// A* Algorithm (Phase 4)
// ============================================================================

bool TrafficSystem::tryDriveAStar(const Position& startPos, ZoneType destZone)
{
    if (!pathfinder_) {
        return false;
    }

    // Create goal predicate that checks for destination zone tiles
    auto isGoal = [this, destZone](int x, int y) -> bool {
        return isDestinationTile(x, y, destZone);
    };

    // Run A* pathfinding
    PathResult result = pathfinder_->findPath(startPos, isGoal);

    if (result.found) {
        lastAStarPath_ = std::move(result.path);
        return true;
    }

    return false;
}

bool TrafficSystem::isDestinationTile(int x, int y, ZoneType destZone) const
{
    if (!mapAccess_.isOnMap(x, y)) {
        return false;
    }

    // Destination tile ranges (same as driveDone)
    static const MapTile targetLow[3] = {COMBASE, LHTHR, LHTHR};
    static const MapTile targetHigh[3] = {NUCLEAR, PORT, COMBASE};

    MapTile l = targetLow[destZone];
    MapTile h = targetHigh[destZone];

    MapTile tile = mapAccess_.getTile(x, y) & TileFlags::LOMASK;
    return (tile >= l && tile <= h);
}

void TrafficSystem::addPathToTrafficDensityMap(const std::vector<Position>& path)
{
    // Update traffic density along path (every other position like original)
    for (size_t i = 0; i < path.size(); i++) {
        // Only update every other position (like original algorithm)
        if (i & 1) {
            const Position& pos = path[i];

            if (mapAccess_.isOnMap(pos.posX, pos.posY)) {
                MapTile tile = mapAccess_.getTile(pos.posX, pos.posY) & TileFlags::LOMASK;

                if (tile >= ROADBASE && tile < POWERBASE) {
                    // Update traffic density
                    int traffic = mapAccess_.getTrafficDensity(pos.posX, pos.posY);
                    traffic += 50;
                    if (traffic > 240) {
                        traffic = 240;
                    }
                    mapAccess_.setTrafficDensity(pos.posX, pos.posY, traffic);

                    // Check for heavy traffic
                    if (traffic >= 240 && (rng_() & 7) == 0) {
                        trafMaxX_ = pos.posX;
                        trafMaxY_ = pos.posY;

                        // Emit heavy traffic event
                        eventBus_.publish(Events::HeavyTraffic{
                            Position(pos.posX, pos.posY),
                            traffic
                        });
                    }
                }
            }
        }
    }
}

} // namespace MicropolisEngine
