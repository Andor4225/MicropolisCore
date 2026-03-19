/**
 * @file PowerSystem.cpp
 * @brief Power grid management subsystem implementation.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from power.cpp. This implementation maintains the original
 * algorithm while providing a cleaner, testable interface.
 */

#include "PowerSystem.h"
#include <cassert>

namespace MicropolisEngine {

PowerSystem::PowerSystem(IMapAccess& mapAccess, EventBus& eventBus)
    : mapAccess_(mapAccess)
    , eventBus_(eventBus)
    , powerStackPointer_(0)
    , initialized_(false)
    , lastPoweredTiles_(0)
    , lastPowerCapacity_(0)
{
}

void PowerSystem::initialize()
{
    powerStackPointer_ = 0;
    lastPoweredTiles_ = 0;
    lastPowerCapacity_ = 0;
    initialized_ = true;
}

void PowerSystem::reset()
{
    powerStackPointer_ = 0;
    mapAccess_.clearPowerGrid();
}

void PowerSystem::tick(int simCycle, int phase)
{
    // Power scan is triggered externally by the simulation loop
    // based on speed settings, so this is currently a no-op.
    // Future: Could auto-scan on specific phases.
    (void)simCycle;
    (void)phase;
}

void PowerSystem::registerPowerPlant(const Position& pos)
{
    pushPowerStack(pos);
}

void PowerSystem::scan(int coalPowerPop, int nuclearPowerPop)
{
    Direction2 anyDir, dir;
    int conNum;

    // Clear power map
    mapAccess_.clearPowerGrid();

    // Calculate total power capacity
    Quad maxPower = calculateCapacity(coalPowerPop, nuclearPowerPop);
    lastPowerCapacity_ = maxPower;

    Quad numPower = 0;  // Amount of power used
    int poweredTiles = 0;

    while (powerStackPointer_ > 0) {
        Position pos = pullPowerStack();
        anyDir = DIR2_INVALID;

        do {
            numPower++;
            poweredTiles++;

            if (numPower > maxPower) {
                // Power demand exceeds supply
                lastPoweredTiles_ = poweredTiles;

                // Emit event for insufficient power
                eventBus_.publish(Events::NotEnoughPower{
                    static_cast<int>(maxPower),
                    static_cast<int>(numPower),
                    static_cast<int>(numPower - maxPower)
                });

                return;
            }

            if (anyDir != DIR2_INVALID) {
                pos.move(anyDir);
            }

            mapAccess_.setPowered(pos.posX, pos.posY, true);

            conNum = 0;
            dir = DIR2_BEGIN;

            while (dir < DIR2_END && conNum < 2) {
                if (testForConductive(pos, dir)) {
                    conNum++;
                    anyDir = dir;
                }
                dir = increment90(dir);
            }

            if (conNum > 1) {
                pushPowerStack(pos);
            }

        } while (conNum);
    }

    lastPoweredTiles_ = poweredTiles;

    // Emit completion event
    eventBus_.publish(Events::PowerScanComplete{
        poweredTiles,
        static_cast<int>(maxPower),
        static_cast<int>(numPower),
        true  // sufficient power
    });
}

bool PowerSystem::hasPower(int x, int y) const
{
    return mapAccess_.isPowered(x, y);
}

void PowerSystem::clearPowerGrid()
{
    powerStackPointer_ = 0;
    mapAccess_.clearPowerGrid();
}

bool PowerSystem::testForConductive(const Position& pos, Direction2 testDir)
{
    Position movedPos(pos);

    if (movedPos.move(testDir)) {
        MapTile tile = mapAccess_.getTile(movedPos.posX, movedPos.posY);

        // Check if tile is conductive
        if ((tile & TileFlags::CONDBIT) == TileFlags::CONDBIT) {
            // Check if tile is not yet powered
            if (!mapAccess_.isPowered(movedPos.posX, movedPos.posY)) {
                return true;
            }
        }
    }

    return false;
}

void PowerSystem::pushPowerStack(const Position& pos)
{
    if (powerStackPointer_ < (POWER_STACK_SIZE - 2)) {
        powerStackPointer_++;
        powerStackXY_[powerStackPointer_] = pos;
    }
}

Position PowerSystem::pullPowerStack()
{
    assert(powerStackPointer_ > 0);
    powerStackPointer_--;
    return powerStackXY_[powerStackPointer_ + 1];
}

} // namespace MicropolisEngine
