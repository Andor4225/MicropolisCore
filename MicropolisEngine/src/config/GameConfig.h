/**
 * @file GameConfig.h
 * @brief Game configuration system for externalizing constants.
 *
 * MODERNIZATION (Phase 4):
 * Provides a centralized configuration system for game constants that were
 * previously hardcoded. Supports loading from JSON files and runtime modification.
 */

#ifndef MICROPOLIS_GAME_CONFIG_H
#define MICROPOLIS_GAME_CONFIG_H

#include <string>
#include <cstdint>

namespace MicropolisEngine {

/**
 * @brief Traffic simulation configuration.
 */
struct TrafficSettings {
    int maxSearchDistance = 30;          ///< Maximum tiles to search for destination
    float trafficCostWeight = 0.0f;      ///< A* traffic density cost weight
    float randomizationFactor = 0.0f;    ///< A* path randomization
    bool useAStar = false;               ///< Use A* instead of random walk
};

/**
 * @brief Power system configuration.
 */
struct PowerSettings {
    int coalPowerStrength = 700;         ///< Power units from coal plant
    int nuclearPowerStrength = 2000;     ///< Power units from nuclear plant
};

/**
 * @brief Budget and tax configuration.
 */
struct BudgetSettings {
    int defaultTaxRate = 7;              ///< Default city tax rate (percent)
    float defaultRoadPercent = 1.0f;     ///< Default road funding percentage
    float defaultPolicePercent = 1.0f;   ///< Default police funding percentage
    float defaultFirePercent = 1.0f;     ///< Default fire funding percentage
    int64_t startingFunds = 20000;       ///< Starting city funds
};

/**
 * @brief Disaster probability configuration.
 */
struct DisasterSettings {
    bool enableDisasters = true;         ///< Master switch for disasters
    int earthquakeFrequency = 480;       ///< Earthquake check frequency (higher = rarer)
    int fireFrequency = 100;             ///< Fire check frequency
    int floodFrequency = 3000;           ///< Flood check frequency
    int tornadoFrequency = 60;           ///< Tornado check frequency
    int meltdownFrequency = 30000;       ///< Meltdown check frequency
};

/**
 * @brief Map/world size configuration.
 *
 * Note: These are initialization-time settings. Changing after init has no effect.
 */
struct WorldSettings {
    int width = 120;                     ///< World width in tiles (default: 120)
    int height = 100;                    ///< World height in tiles (default: 100)
};

/**
 * @brief Simulation speed and timing configuration.
 */
struct SimulationSettings {
    int censusFrequency = 4;             ///< Months between census updates
    int taxFrequency = 48;               ///< Months between tax collection
    int evaluationFrequency = 12;        ///< Months between city evaluation
};

/**
 * @brief Master game configuration class.
 *
 * Holds all configurable game parameters. Can be loaded from JSON or
 * modified programmatically before game initialization.
 *
 * Usage:
 * @code
 *   GameConfig config;
 *   config.loadFromFile("game_config.json");
 *
 *   // Or modify directly
 *   config.traffic.maxSearchDistance = 50;
 *   config.traffic.useAStar = true;
 *
 *   Micropolis game;
 *   game.applyConfig(config);
 * @endcode
 */
class GameConfig {
public:
    GameConfig() = default;
    ~GameConfig() = default;

    /**
     * @brief Load configuration from a JSON file.
     * @param filename Path to the JSON configuration file.
     * @return True if loaded successfully, false on error.
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief Save current configuration to a JSON file.
     * @param filename Path to save the configuration.
     * @return True if saved successfully, false on error.
     */
    bool saveToFile(const std::string& filename) const;

    /**
     * @brief Load configuration from a JSON string.
     * @param jsonString JSON configuration string.
     * @return True if parsed successfully, false on error.
     */
    bool loadFromString(const std::string& jsonString);

    /**
     * @brief Export configuration to a JSON string.
     * @return JSON string representation of the configuration.
     */
    std::string toJsonString() const;

    /**
     * @brief Reset all settings to defaults.
     */
    void resetToDefaults();

    /**
     * @brief Get the last error message if loading failed.
     * @return Error message string.
     */
    const std::string& getLastError() const { return lastError_; }

    // Configuration sections
    TrafficSettings traffic;
    PowerSettings power;
    BudgetSettings budget;
    DisasterSettings disasters;
    WorldSettings world;
    SimulationSettings simulation;

private:
    std::string lastError_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_GAME_CONFIG_H
