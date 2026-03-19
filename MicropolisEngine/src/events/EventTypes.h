/**
 * @file EventTypes.h
 * @brief Event struct definitions for the Micropolis event bus.
 *
 * MODERNIZATION (Phase 3):
 * These event types enable decoupled communication between subsystems.
 * Each event carries the relevant data for its domain.
 */

#ifndef MICROPOLIS_EVENT_TYPES_H
#define MICROPOLIS_EVENT_TYPES_H

#include "../data_types.h"
#include "../map_type.h"   // Must be before position.h for WORLD_W/WORLD_H
#include "../position.h"

namespace MicropolisEngine {
namespace Events {

// ============================================================================
// Power System Events
// ============================================================================

/**
 * @brief Emitted when power grid scan completes.
 */
struct PowerScanComplete {
    int poweredTiles;       ///< Number of tiles receiving power
    int totalPowerCapacity; ///< Total power generation capacity
    int powerConsumed;      ///< Power consumed by zones
    bool sufficient;        ///< True if power meets demand
};

/**
 * @brief Emitted when power demand exceeds supply.
 */
struct NotEnoughPower {
    int available;  ///< Available power capacity
    int required;   ///< Required power demand
    int deficit;    ///< Power shortage amount
};

/**
 * @brief Emitted when a power plant is registered.
 */
struct PowerPlantRegistered {
    Position location;
    bool isNuclear;  ///< True for nuclear, false for coal
    int capacity;    ///< Power generation capacity
};

// ============================================================================
// Traffic System Events
// ============================================================================

/**
 * @brief Emitted when heavy traffic is detected.
 */
struct HeavyTraffic {
    Position location;
    int density;     ///< Traffic density at location (0-255)
};

/**
 * @brief Emitted when traffic pathfinding completes.
 */
struct TrafficRouteComplete {
    Position start;
    Position end;
    int pathLength;
    bool successful;
};

/**
 * @brief Emitted when traffic density map is updated.
 */
struct TrafficDensityUpdated {
    int averageDensity;
    Position hotspot;
};

// ============================================================================
// Budget System Events
// ============================================================================

/**
 * @brief Emitted when budget allocation is updated.
 */
struct BudgetUpdated {
    Funds roadSpend;
    Funds policeSpend;
    Funds fireSpend;
    Funds roadEffect;
    Funds policeEffect;
    Funds fireEffect;
};

/**
 * @brief Emitted when city runs out of money.
 */
struct NoMoney {
    Funds deficit;  ///< How much the city is short
};

/**
 * @brief Emitted when tax rate changes.
 */
struct TaxRateChanged {
    short oldRate;
    short newRate;
};

/**
 * @brief Emitted when funds change significantly.
 */
struct FundsChanged {
    Funds oldAmount;
    Funds newAmount;
    Funds delta;
};

// ============================================================================
// Disaster System Events
// ============================================================================

/**
 * @brief Disaster type enumeration.
 */
enum class DisasterType {
    Fire,
    Flood,
    Earthquake,
    Meltdown,
    Monster,
    Tornado,
    FireBombs
};

/**
 * @brief Emitted when a disaster starts.
 */
struct DisasterStarted {
    DisasterType type;
    Position location;
    int severity;  ///< Disaster intensity (for earthquakes: 0-1000)
};

/**
 * @brief Emitted when a disaster ends.
 */
struct DisasterEnded {
    DisasterType type;
    int tilesAffected;
};

/**
 * @brief Emitted when flooding spreads.
 */
struct FloodSpreading {
    Position location;
    int floodCount;  ///< Remaining flood duration
};

// ============================================================================
// Evaluation System Events
// ============================================================================

/**
 * @brief Emitted when city evaluation completes.
 */
struct EvaluationComplete {
    Population cityPop;
    Population cityPopDelta;
    short cityScore;
    short cityScoreDelta;
    short cityYes;       ///< Mayor approval rating (0-100)
    int cityClass;       ///< CityClass enum value
};

/**
 * @brief Emitted when city class changes.
 */
struct CityClassChanged {
    int oldClass;
    int newClass;
    Population population;
};

/**
 * @brief Emitted when problems are identified.
 */
struct ProblemsIdentified {
    short problemVotes[7];   ///< Votes for each problem type
    short topProblems[3];    ///< Indices of top 3 problems
};

// ============================================================================
// Zone System Events
// ============================================================================

/**
 * @brief Emitted when a zone grows.
 */
struct ZoneGrowth {
    Position location;
    int zoneType;       ///< 0=residential, 1=commercial, 2=industrial
    int newDensity;
};

/**
 * @brief Emitted when a zone declines.
 */
struct ZoneDecline {
    Position location;
    int zoneType;
    int newDensity;
};

/**
 * @brief Emitted when a zone becomes powered/unpowered.
 */
struct ZonePowerChanged {
    Position location;
    bool powered;
};

// ============================================================================
// General Game Events
// ============================================================================

/**
 * @brief General game message event.
 * Replaces sendMessage() callback for many use cases.
 */
struct GameMessage {
    short messageId;
    Position location;
    bool showPicture;
    bool important;
};

/**
 * @brief Emitted at the start of each simulation tick.
 */
struct SimulationTick {
    int cityTime;
    int simCycle;
    int phase;
};

/**
 * @brief Emitted when simulation speed changes.
 */
struct SpeedChanged {
    int oldSpeed;
    int newSpeed;
};

/**
 * @brief Emitted when a map scan phase completes.
 */
struct MapScanComplete {
    int phase;
    int zonesProcessed;
};

} // namespace Events
} // namespace MicropolisEngine

#endif // MICROPOLIS_EVENT_TYPES_H
