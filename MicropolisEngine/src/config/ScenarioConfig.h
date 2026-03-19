/**
 * @file ScenarioConfig.h
 * @brief Scenario configuration and loading system.
 *
 * MODERNIZATION (Phase 6.5):
 * Provides JSON-based scenario definition for custom game scenarios.
 * Scenarios define starting conditions, victory conditions, and special rules.
 */

#ifndef MICROPOLIS_SCENARIO_CONFIG_H
#define MICROPOLIS_SCENARIO_CONFIG_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace MicropolisEngine {

/**
 * @brief Disaster type for scenario events.
 */
enum class ScenarioDisaster {
    None,
    Fire,
    Flood,
    Tornado,
    Earthquake,
    Monster,
    Meltdown
};

/**
 * @brief Victory condition types.
 */
enum class VictoryType {
    Population,     ///< Reach target population
    Score,          ///< Reach target score
    Funds,          ///< Accumulate target funds
    SurviveTime,    ///< Survive for specified time
    Custom          ///< Custom win condition
};

/**
 * @brief A scheduled event in the scenario timeline.
 */
struct ScenarioEvent {
    int triggerTime = 0;                    ///< City time to trigger event
    ScenarioDisaster disaster = ScenarioDisaster::None;
    int locationX = -1;                     ///< -1 = random location
    int locationY = -1;
    std::string message;                    ///< Optional message to display
};

/**
 * @brief Victory condition definition.
 */
struct VictoryCondition {
    VictoryType type = VictoryType::Population;
    int64_t targetValue = 0;                ///< Target to achieve
    int timeLimit = 0;                      ///< 0 = no time limit
    std::string description;
};

/**
 * @brief Starting city configuration.
 */
struct StartingConfig {
    int64_t funds = 20000;                  ///< Starting funds
    int taxRate = 7;                        ///< Starting tax rate
    int worldWidth = 120;                   ///< Map width
    int worldHeight = 100;                  ///< Map height
    int64_t seed = 0;                       ///< Map generation seed (0 = random)
    bool generateCity = false;              ///< Auto-generate starting city
    std::string loadFile;                   ///< Pre-built city file to load
};

/**
 * @brief Scenario rules and modifiers.
 */
struct ScenarioRules {
    bool disastersEnabled = true;
    bool autoGoto = true;                   ///< Auto-center on events
    bool autoBudget = false;
    float pollutionMultiplier = 1.0f;
    float crimeMultiplier = 1.0f;
    float trafficMultiplier = 1.0f;
    float growthMultiplier = 1.0f;
    int maxGameSpeed = 3;                   ///< Maximum allowed game speed
};

/**
 * @brief Complete scenario definition.
 */
struct ScenarioConfig {
    // Metadata
    std::string id;                         ///< Unique scenario identifier
    std::string name;                       ///< Display name
    std::string description;                ///< Long description
    std::string author;
    std::string version = "1.0";
    int difficulty = 1;                     ///< 1-5 difficulty rating

    // Configuration
    StartingConfig starting;
    ScenarioRules rules;
    std::vector<VictoryCondition> victories;
    std::vector<ScenarioEvent> events;

    /**
     * @brief Check if scenario has a valid structure.
     */
    bool isValid() const {
        return !id.empty() && !name.empty() && !victories.empty();
    }
};

/**
 * @brief Result of loading a scenario.
 */
enum class ScenarioLoadResult {
    Success,
    FileNotFound,
    ParseError,
    InvalidFormat,
    MissingRequiredField,
    VersionMismatch
};

/**
 * @brief Scenario loader class.
 */
class ScenarioLoader {
public:
    /**
     * @brief Load scenario from JSON file.
     * @param filePath Path to JSON scenario file.
     * @param[out] config Loaded configuration.
     * @return Load result status.
     */
    static ScenarioLoadResult loadFromFile(
        const std::string& filePath,
        ScenarioConfig& config);

    /**
     * @brief Load scenario from JSON string.
     * @param jsonContent JSON content as string.
     * @param[out] config Loaded configuration.
     * @return Load result status.
     */
    static ScenarioLoadResult loadFromString(
        const std::string& jsonContent,
        ScenarioConfig& config);

    /**
     * @brief Save scenario to JSON file.
     * @param filePath Output file path.
     * @param config Configuration to save.
     * @return True if successful.
     */
    static bool saveToFile(
        const std::string& filePath,
        const ScenarioConfig& config);

    /**
     * @brief Export scenario to JSON string.
     * @param config Configuration to export.
     * @return JSON string representation.
     */
    static std::string exportToString(const ScenarioConfig& config);

    /**
     * @brief Get list of available scenarios in a directory.
     * @param directory Path to scenario directory.
     * @return Vector of scenario file paths.
     */
    static std::vector<std::string> listScenarios(const std::string& directory);

    /**
     * @brief Get human-readable error message.
     * @param result Load result code.
     * @return Error description.
     */
    static std::string getErrorMessage(ScenarioLoadResult result);
};

/**
 * @brief Runtime scenario state tracker.
 *
 * Tracks progress toward victory conditions and manages scheduled events.
 */
class ScenarioRunner {
public:
    ScenarioRunner();

    /**
     * @brief Initialize with a scenario configuration.
     * @param config Scenario to run.
     */
    void initialize(const ScenarioConfig& config);

    /**
     * @brief Update scenario state.
     * @param cityTime Current city time.
     * @param population Current population.
     * @param score Current city score.
     * @param funds Current funds.
     */
    void update(int64_t cityTime, int64_t population, int score, int64_t funds);

    /**
     * @brief Check if victory has been achieved.
     * @return True if any victory condition is met.
     */
    bool hasWon() const { return won_; }

    /**
     * @brief Check if defeat condition is met (time expired).
     * @return True if time limit exceeded without victory.
     */
    bool hasLost() const { return lost_; }

    /**
     * @brief Get pending events that should trigger.
     * @return Events ready to trigger (clears the pending list).
     */
    std::vector<ScenarioEvent> getPendingEvents();

    /**
     * @brief Get progress toward first victory condition (0.0 - 1.0).
     */
    float getProgress() const { return progress_; }

    /**
     * @brief Get remaining time if time-limited.
     * @return Remaining time, or -1 if no time limit.
     */
    int getRemainingTime() const;

    /**
     * @brief Check if scenario is active.
     */
    bool isActive() const { return active_; }

    /**
     * @brief Reset scenario state.
     */
    void reset();

private:
    ScenarioConfig config_;
    bool active_ = false;
    bool won_ = false;
    bool lost_ = false;
    float progress_ = 0.0f;
    size_t nextEventIndex_ = 0;
    std::vector<ScenarioEvent> pendingEvents_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_SCENARIO_CONFIG_H
