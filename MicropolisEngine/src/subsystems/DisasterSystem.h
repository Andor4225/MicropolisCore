/**
 * @file DisasterSystem.h
 * @brief Disaster management subsystem.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from disasters.cpp to provide a standalone, testable disaster
 * system. The DisasterSystem handles:
 * - Random disaster triggering based on game level
 * - Scenario-specific disaster scheduling
 * - Earthquake, flood, fire, and other disaster effects
 */

#ifndef MICROPOLIS_DISASTER_SYSTEM_H
#define MICROPOLIS_DISASTER_SYSTEM_H

#include "ISubsystem.h"
#include "IMapAccess.h"
#include "../events/EventBus.h"
#include "../events/EventTypes.h"
#include "../position.h"
#include "../data_types.h"
#include <functional>

namespace MicropolisEngine {

/**
 * @brief Game difficulty level enumeration.
 */
enum class GameLevel {
    Easy = 0,
    Medium = 1,
    Hard = 2
};

/**
 * @brief Scenario type enumeration.
 */
enum class ScenarioType {
    None = 0,
    Dullsville = 1,
    SanFrancisco = 2,
    Hamburg = 3,
    Bern = 4,
    Tokyo = 5,
    Detroit = 6,
    Boston = 7,
    Rio = 8
};

/**
 * @brief Disaster management subsystem.
 *
 * Manages random and scenario-based disasters including earthquakes,
 * floods, fires, tornados, and monster attacks.
 *
 * Usage:
 * @code
 *   DisasterSystem disasters(mapAccess, eventBus,
 *                            [this]() { return getRandom16(); },
 *                            [this](short n) { return getRandom(n); });
 *   disasters.initialize();
 *
 *   // Call each simulation tick
 *   disasters.update(disastersEnabled, GameLevel::Medium,
 *                    pollutionAverage, scenario, disasterWait);
 *
 *   // Trigger specific disasters manually
 *   disasters.triggerEarthquake(cityCenterX, cityCenterY);
 * @endcode
 */
class DisasterSystem : public ITickableSubsystem {
public:
    /** Random number generator function type. */
    using Rng16Function = std::function<int()>;
    using RngRangeFunction = std::function<short(short)>;

    /**
     * @brief Construct a DisasterSystem with injected dependencies.
     * @param mapAccess Interface for map read/write operations.
     * @param eventBus Event bus for publishing disaster events.
     * @param rng16 Random number generator (0-65535).
     * @param rngRange Random number generator for range [0, n).
     */
    DisasterSystem(IMapAccess& mapAccess, EventBus& eventBus,
                   Rng16Function rng16, RngRangeFunction rngRange);

    ~DisasterSystem() override = default;

    // ISubsystem interface
    const char* getName() const override { return "DisasterSystem"; }
    void initialize() override;
    void reset() override;
    bool isInitialized() const override { return initialized_; }

    // ITickableSubsystem interface
    void tick(int simCycle, int phase) override;

    /**
     * @brief Update disasters for this simulation step.
     *
     * Handles random disasters and scenario-specific disasters.
     *
     * @param enabled True if disasters are enabled.
     * @param level Current game difficulty level.
     * @param pollutionAvg Average pollution level (affects monster chance).
     * @param scenario Current scenario type.
     * @param disasterWait Countdown for scenario disasters.
     * @return Updated disaster wait countdown.
     */
    int update(bool enabled, GameLevel level, short pollutionAvg,
               ScenarioType scenario, int disasterWait);

    /**
     * @brief Trigger an earthquake.
     * @param centerX X coordinate of city center (for message).
     * @param centerY Y coordinate of city center (for message).
     */
    void triggerEarthquake(int centerX, int centerY);

    /**
     * @brief Trigger flooding.
     */
    void triggerFlood();

    /**
     * @brief Trigger a fire at random location.
     */
    void triggerFire();

    /**
     * @brief Trigger fire bombs (for Hamburg scenario).
     */
    void triggerFireBombs();

    /**
     * @brief Trigger nuclear meltdown.
     * @return True if a nuclear plant was found and melted down.
     */
    bool triggerMeltdown();

    /**
     * @brief Process flooding spread/cleanup for current tick.
     */
    void processFlood(const Position& pos);

    /**
     * @brief Get current flood countdown.
     * @return Remaining flood ticks.
     */
    int getFloodCount() const { return floodCount_; }

    /**
     * @brief Set flood countdown (e.g., when starting new flood).
     * @param count Number of ticks for flood.
     */
    void setFloodCount(int count) { floodCount_ = count; }

private:
    /**
     * @brief Check if a tile is vulnerable to earthquake damage.
     * @param tileValue Full tile value including flags.
     * @return True if vulnerable.
     */
    bool isVulnerable(MapTile tileValue) const;

    /**
     * @brief Generate a random rubble tile.
     * @return Rubble tile value with appropriate flags.
     */
    MapTile randomRubble() const;

    /**
     * @brief Generate a random fire tile.
     * @return Fire tile value with appropriate flags.
     */
    MapTile randomFire() const;

    IMapAccess& mapAccess_;
    EventBus& eventBus_;
    Rng16Function rng16_;
    RngRangeFunction rngRange_;

    int floodCount_;
    bool initialized_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_DISASTER_SYSTEM_H
