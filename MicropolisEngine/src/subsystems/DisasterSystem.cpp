/**
 * @file DisasterSystem.cpp
 * @brief Disaster management subsystem implementation.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from disasters.cpp. This implementation maintains the original
 * disaster logic while providing a cleaner, testable interface.
 */

#include "DisasterSystem.h"
#include <cassert>

// Tile constants needed for disaster system
namespace {
    constexpr MapTile DIRT        = 0;
    constexpr MapTile RUBBLE      = 44;
    constexpr MapTile LASTRUBBLE  = 47;
    constexpr MapTile FLOOD       = 48;
    constexpr MapTile FIRE        = 56;
    constexpr MapTile LASTFIRE    = 63;
    constexpr MapTile RESBASE     = 240;
    constexpr MapTile LASTZONE    = 956;
    constexpr MapTile CHANNEL     = 19;     // River channel
    constexpr MapTile WATER_HIGH  = 21;     // Highest water tile
    constexpr MapTile WOODS5      = 43;
    constexpr MapTile NUCLEAR     = 816;

    // Tile flags
    constexpr MapTile ZONEBIT     = 0x0400;
    constexpr MapTile BURNBIT     = 0x2000;
    constexpr MapTile BULLBIT     = 0x1000;
    constexpr MapTile LOMASK      = 0x03FF;
    constexpr MapTile ANIMBIT     = 0x0800;
}

namespace MicropolisEngine {

DisasterSystem::DisasterSystem(IMapAccess& mapAccess, EventBus& eventBus,
                               Rng16Function rng16, RngRangeFunction rngRange)
    : mapAccess_(mapAccess)
    , eventBus_(eventBus)
    , rng16_(std::move(rng16))
    , rngRange_(std::move(rngRange))
    , floodCount_(0)
    , initialized_(false)
{
}

void DisasterSystem::initialize()
{
    floodCount_ = 0;
    initialized_ = true;
}

void DisasterSystem::reset()
{
    floodCount_ = 0;
}

void DisasterSystem::tick(int simCycle, int phase)
{
    // Disasters are triggered by the main simulation loop calling update()
    (void)simCycle;
    (void)phase;
}

int DisasterSystem::update(bool enabled, GameLevel level, short pollutionAvg,
                           ScenarioType scenario, int disasterWait)
{
    // Disaster chance based on game level
    static const short DisChance[3] = {
        10 * 48,  // Easy
        5 * 48,   // Medium
        60        // Hard
    };

    // Decrement flood counter
    if (floodCount_ > 0) {
        floodCount_--;
    }

    // Handle scenario-specific disasters
    if (scenario != ScenarioType::None && disasterWait > 0) {
        switch (scenario) {
            case ScenarioType::SanFrancisco:
                if (disasterWait == 1) {
                    eventBus_.publish(Events::DisasterStarted{
                        Events::DisasterType::Earthquake,
                        Position(mapAccess_.getWorldWidth() / 2,
                                mapAccess_.getWorldHeight() / 2),
                        500  // Medium earthquake for scenario
                    });
                }
                break;

            case ScenarioType::Hamburg:
                if ((disasterWait % 10) == 0) {
                    eventBus_.publish(Events::DisasterStarted{
                        Events::DisasterType::FireBombs,
                        Position(rngRange_(mapAccess_.getWorldWidth()),
                                rngRange_(mapAccess_.getWorldHeight())),
                        3  // 2-3 bombs
                    });
                }
                break;

            case ScenarioType::Tokyo:
                if (disasterWait == 1) {
                    eventBus_.publish(Events::DisasterStarted{
                        Events::DisasterType::Monster,
                        Position(rngRange_(mapAccess_.getWorldWidth()),
                                rngRange_(mapAccess_.getWorldHeight())),
                        1
                    });
                }
                break;

            case ScenarioType::Boston:
                if (disasterWait == 1) {
                    triggerMeltdown();
                }
                break;

            case ScenarioType::Rio:
                if ((disasterWait % 24) == 0) {
                    triggerFlood();
                }
                break;

            default:
                break;
        }

        disasterWait--;
    }

    if (!enabled) {
        return disasterWait;
    }

    int levelIdx = static_cast<int>(level);
    if (levelIdx > 2) {
        levelIdx = 0;
    }

    // Random disaster check
    if (rngRange_(DisChance[levelIdx]) == 0) {
        int disasterType = rngRange_(9);

        switch (disasterType) {
            case 0:
            case 1:
                // Fire (2/9 chance)
                triggerFire();
                break;

            case 2:
            case 3:
                // Flood (2/9 chance)
                triggerFlood();
                break;

            case 4:
                // Nothing (was airplane crash - removed after 9/11)
                break;

            case 5:
                // Tornado (1/9 chance)
                eventBus_.publish(Events::DisasterStarted{
                    Events::DisasterType::Tornado,
                    Position(rngRange_(mapAccess_.getWorldWidth()),
                            rngRange_(mapAccess_.getWorldHeight())),
                    1
                });
                break;

            case 6:
                // Earthquake (1/9 chance)
                triggerEarthquake(mapAccess_.getWorldWidth() / 2,
                                  mapAccess_.getWorldHeight() / 2);
                break;

            case 7:
            case 8:
                // Monster (2/9 chance if polluted)
                if (pollutionAvg > 60) {
                    eventBus_.publish(Events::DisasterStarted{
                        Events::DisasterType::Monster,
                        Position(rngRange_(mapAccess_.getWorldWidth()),
                                rngRange_(mapAccess_.getWorldHeight())),
                        1
                    });
                }
                break;
        }
    }

    return disasterWait;
}

void DisasterSystem::triggerEarthquake(int centerX, int centerY)
{
    int strength = rngRange_(700) + 300;

    eventBus_.publish(Events::DisasterStarted{
        Events::DisasterType::Earthquake,
        Position(centerX, centerY),
        strength
    });

    // Damage random tiles
    int worldW = mapAccess_.getWorldWidth();
    int worldH = mapAccess_.getWorldHeight();

    for (int z = 0; z < strength; z++) {
        int x = rngRange_(worldW);
        int y = rngRange_(worldH);

        MapTile tile = mapAccess_.getTile(x, y);
        if (isVulnerable(tile)) {
            if ((z & 0x3) != 0) {
                // 3 of 4 times reduce to rubble
                mapAccess_.setTile(x, y, randomRubble());
            } else {
                // 1 of 4 times start fire
                mapAccess_.setTile(x, y, randomFire());
            }
        }
    }
}

void DisasterSystem::triggerFlood()
{
    static const short Dx[4] = { 0, 1, 0, -1 };
    static const short Dy[4] = { -1, 0, 1, 0 };

    int worldW = mapAccess_.getWorldWidth();
    int worldH = mapAccess_.getWorldHeight();

    for (int z = 0; z < 300; z++) {
        int x = rngRange_(worldW);
        int y = rngRange_(worldH);
        MapTile tile = mapAccess_.getTile(x, y) & LOMASK;

        // Check for river edge
        if (tile > CHANNEL && tile <= WATER_HIGH) {
            for (int t = 0; t < 4; t++) {
                int xx = x + Dx[t];
                int yy = y + Dy[t];

                if (mapAccess_.isOnMap(xx, yy)) {
                    MapTile c = mapAccess_.getTile(xx, yy);

                    // Tile is floodable
                    if ((c & LOMASK) == DIRT ||
                        (c & (BULLBIT | BURNBIT)) == (BULLBIT | BURNBIT)) {

                        mapAccess_.setTile(xx, yy, FLOOD);
                        floodCount_ = 30;

                        eventBus_.publish(Events::DisasterStarted{
                            Events::DisasterType::Flood,
                            Position(xx, yy),
                            30
                        });

                        return;
                    }
                }
            }
        }
    }
}

void DisasterSystem::triggerFire()
{
    int worldW = mapAccess_.getWorldWidth();
    int worldH = mapAccess_.getWorldHeight();

    int x = rngRange_(worldW);
    int y = rngRange_(worldH);
    MapTile tile = mapAccess_.getTile(x, y);
    MapTile tileBase = tile & LOMASK;

    // Only start fire on non-zone, non-trivial tiles
    if (!(tile & ZONEBIT) && tileBase > 249 && tileBase < LASTZONE) {
        mapAccess_.setTile(x, y, randomFire());

        eventBus_.publish(Events::DisasterStarted{
            Events::DisasterType::Fire,
            Position(x, y),
            1
        });
    }
}

void DisasterSystem::triggerFireBombs()
{
    int count = 2 + (rng16_() & 1);

    eventBus_.publish(Events::DisasterStarted{
        Events::DisasterType::FireBombs,
        Position(rngRange_(mapAccess_.getWorldWidth()),
                rngRange_(mapAccess_.getWorldHeight())),
        count
    });
}

bool DisasterSystem::triggerMeltdown()
{
    int worldW = mapAccess_.getWorldWidth();
    int worldH = mapAccess_.getWorldHeight();

    for (int x = 0; x < worldW - 1; x++) {
        for (int y = 0; y < worldH - 1; y++) {
            if ((mapAccess_.getTile(x, y) & LOMASK) == NUCLEAR) {
                eventBus_.publish(Events::DisasterStarted{
                    Events::DisasterType::Meltdown,
                    Position(x, y),
                    1
                });
                return true;
            }
        }
    }

    return false;
}

void DisasterSystem::processFlood(const Position& pos)
{
    static const short Dx[4] = { 0, 1, 0, -1 };
    static const short Dy[4] = { -1, 0, 1, 0 };

    if (floodCount_ > 0) {
        // Flood is spreading
        for (int z = 0; z < 4; z++) {
            if ((rng16_() & 7) == 0) {  // 12.5% chance per direction
                int xx = pos.posX + Dx[z];
                int yy = pos.posY + Dy[z];

                if (mapAccess_.isOnMap(xx, yy)) {
                    MapTile c = mapAccess_.getTile(xx, yy);
                    MapTile t = c & LOMASK;

                    if ((c & BURNBIT) || (c & LOMASK) == DIRT ||
                        (t >= WOODS5 && t < FLOOD)) {

                        mapAccess_.setTile(xx, yy, FLOOD + rngRange_(3));

                        eventBus_.publish(Events::FloodSpreading{
                            Position(xx, yy),
                            floodCount_
                        });
                    }
                }
            }
        }
    } else {
        // Flood is receding
        if ((rng16_() & 15) == 0) {  // 1/16 chance to clear
            mapAccess_.setTile(pos.posX, pos.posY, DIRT);
        }
    }
}

bool DisasterSystem::isVulnerable(MapTile tileValue) const
{
    MapTile tileBase = tileValue & LOMASK;

    if (tileBase < RESBASE || tileBase > LASTZONE || (tileValue & ZONEBIT)) {
        return false;
    }

    return true;
}

MapTile DisasterSystem::randomRubble() const
{
    return RUBBLE + rngRange_(4) + BULLBIT;
}

MapTile DisasterSystem::randomFire() const
{
    return FIRE + rngRange_(8) + ANIMBIT + BURNBIT;
}

} // namespace MicropolisEngine
