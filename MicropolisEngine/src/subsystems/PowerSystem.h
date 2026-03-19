/**
 * @file PowerSystem.h
 * @brief Power grid management subsystem.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from power.cpp to provide a standalone, testable power
 * distribution system. The PowerSystem manages:
 * - Power plant registration
 * - Power grid scanning and distribution
 * - Power status queries
 */

#ifndef MICROPOLIS_POWER_SYSTEM_H
#define MICROPOLIS_POWER_SYSTEM_H

#include "ISubsystem.h"
#include "IMapAccess.h"
#include "../events/EventBus.h"
#include "../events/EventTypes.h"
#include "../position.h"
#include "../data_types.h"

namespace MicropolisEngine {

/** Size of the power stack for graph traversal. */
constexpr int POWER_STACK_SIZE = 12000 / 4;  // WORLD_W * WORLD_H / 4

/** Power capacity per coal power plant. */
constexpr Quad COAL_POWER_STRENGTH = 700L;

/** Power capacity per nuclear power plant. */
constexpr Quad NUCLEAR_POWER_STRENGTH = 2000L;

/**
 * @brief Power grid management subsystem.
 *
 * Handles power distribution from power plants to zones across the map.
 * Uses a stack-based flood-fill algorithm to trace conductive paths.
 *
 * Usage:
 * @code
 *   PowerSystem power(mapAccess, eventBus);
 *   power.initialize();
 *
 *   // Register power plants found during map scan
 *   power.registerPowerPlant(Position(10, 10), false);  // Coal plant
 *
 *   // Perform power distribution scan
 *   power.scan(coalPowerPop, nuclearPowerPop);
 *
 *   // Query power status
 *   if (power.hasPower(x, y)) { ... }
 * @endcode
 */
class PowerSystem : public ITickableSubsystem {
public:
    /**
     * @brief Construct a PowerSystem with injected dependencies.
     * @param mapAccess Interface for map read/write operations.
     * @param eventBus Event bus for publishing power events.
     */
    PowerSystem(IMapAccess& mapAccess, EventBus& eventBus);

    ~PowerSystem() override = default;

    // ISubsystem interface
    const char* getName() const override { return "PowerSystem"; }
    void initialize() override;
    void reset() override;
    bool isInitialized() const override { return initialized_; }

    // ITickableSubsystem interface
    void tick(int simCycle, int phase) override;

    /**
     * @brief Register a power plant position for the next scan.
     *
     * Call this when a power plant tile is found during map scanning.
     * The position is pushed onto the power stack for processing
     * during the next doPowerScan() call.
     *
     * @param pos Position of the power plant.
     */
    void registerPowerPlant(const Position& pos);

    /**
     * @brief Perform power distribution scan.
     *
     * Clears the power grid and traces power from all registered
     * power plants through conductive tiles. Emits NotEnoughPower
     * event if demand exceeds supply.
     *
     * @param coalPowerPop Number of coal power plants.
     * @param nuclearPowerPop Number of nuclear power plants.
     */
    void scan(int coalPowerPop, int nuclearPowerPop);

    /**
     * @brief Check if a tile has power.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return True if the tile is powered.
     */
    bool hasPower(int x, int y) const;

    /**
     * @brief Clear the power grid and reset the power stack.
     */
    void clearPowerGrid();

    /**
     * @brief Get the current power stack depth.
     * @return Number of positions on the power stack.
     */
    int getStackDepth() const { return powerStackPointer_; }

    /**
     * @brief Get the power capacity.
     * @param coalPowerPop Number of coal power plants.
     * @param nuclearPowerPop Number of nuclear power plants.
     * @return Total power generation capacity.
     */
    static Quad calculateCapacity(int coalPowerPop, int nuclearPowerPop) {
        return coalPowerPop * COAL_POWER_STRENGTH +
               nuclearPowerPop * NUCLEAR_POWER_STRENGTH;
    }

private:
    /**
     * @brief Test for conductive tile in a direction.
     * @param pos Starting position.
     * @param testDir Direction to check.
     * @return True if an unpowered conductive tile exists in that direction.
     */
    bool testForConductive(const Position& pos, Direction2 testDir);

    /**
     * @brief Push a position onto the power stack.
     * @param pos Position to push.
     */
    void pushPowerStack(const Position& pos);

    /**
     * @brief Pull a position from the power stack.
     * @return Position from the top of the stack.
     */
    Position pullPowerStack();

    IMapAccess& mapAccess_;
    EventBus& eventBus_;

    int powerStackPointer_;
    Position powerStackXY_[POWER_STACK_SIZE];

    bool initialized_;
    int lastPoweredTiles_;
    Quad lastPowerCapacity_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_POWER_SYSTEM_H
