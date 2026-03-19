/**
 * @file GameConfig.cpp
 * @brief Game configuration implementation.
 *
 * MODERNIZATION (Phase 4):
 * Implements JSON loading/saving for game configuration.
 * Uses a minimal parser to avoid external dependencies.
 */

#include "GameConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace MicropolisEngine {

// ============================================================================
// Minimal JSON Parser (no external dependencies)
// ============================================================================

namespace {

/**
 * @brief Skip whitespace in a string.
 */
size_t skipWhitespace(const std::string& s, size_t pos) {
    while (pos < s.length() && std::isspace(static_cast<unsigned char>(s[pos]))) {
        pos++;
    }
    return pos;
}

/**
 * @brief Parse a JSON string value.
 */
bool parseString(const std::string& json, size_t& pos, std::string& value) {
    pos = skipWhitespace(json, pos);
    if (pos >= json.length() || json[pos] != '"') {
        return false;
    }
    pos++;  // Skip opening quote

    value.clear();
    while (pos < json.length() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.length()) {
            pos++;
            switch (json[pos]) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default: value += json[pos]; break;
            }
        } else {
            value += json[pos];
        }
        pos++;
    }

    if (pos >= json.length()) {
        return false;
    }
    pos++;  // Skip closing quote
    return true;
}

/**
 * @brief Parse a JSON number value.
 */
bool parseNumber(const std::string& json, size_t& pos, double& value) {
    pos = skipWhitespace(json, pos);
    size_t start = pos;

    // Handle negative numbers
    if (pos < json.length() && json[pos] == '-') {
        pos++;
    }

    // Parse digits
    while (pos < json.length() && (std::isdigit(static_cast<unsigned char>(json[pos])) ||
                                    json[pos] == '.' || json[pos] == 'e' || json[pos] == 'E' ||
                                    json[pos] == '+' || json[pos] == '-')) {
        pos++;
    }

    if (pos == start) {
        return false;
    }

    try {
        value = std::stod(json.substr(start, pos - start));
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * @brief Parse a JSON boolean value.
 */
bool parseBool(const std::string& json, size_t& pos, bool& value) {
    pos = skipWhitespace(json, pos);

    if (json.compare(pos, 4, "true") == 0) {
        value = true;
        pos += 4;
        return true;
    } else if (json.compare(pos, 5, "false") == 0) {
        value = false;
        pos += 5;
        return true;
    }
    return false;
}

/**
 * @brief Find a key in a JSON object starting at pos.
 */
bool findKey(const std::string& json, size_t& pos, const std::string& key) {
    pos = skipWhitespace(json, pos);

    while (pos < json.length()) {
        // Find next string (potential key)
        while (pos < json.length() && json[pos] != '"') {
            pos++;
        }
        if (pos >= json.length()) {
            return false;
        }

        std::string foundKey;
        if (!parseString(json, pos, foundKey)) {
            return false;
        }

        // Skip to colon
        pos = skipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != ':') {
            return false;
        }
        pos++;
        pos = skipWhitespace(json, pos);

        if (foundKey == key) {
            return true;
        }

        // Skip this value
        int depth = 0;
        bool inString = false;
        while (pos < json.length()) {
            if (json[pos] == '"' && (pos == 0 || json[pos-1] != '\\')) {
                inString = !inString;
            } else if (!inString) {
                if (json[pos] == '{' || json[pos] == '[') {
                    depth++;
                } else if (json[pos] == '}' || json[pos] == ']') {
                    if (depth == 0) break;
                    depth--;
                } else if (json[pos] == ',' && depth == 0) {
                    pos++;
                    break;
                }
            }
            pos++;
        }
    }
    return false;
}

/**
 * @brief Find an object section in JSON.
 */
bool findObject(const std::string& json, const std::string& key, size_t& start, size_t& end) {
    size_t pos = 0;
    if (!findKey(json, pos, key)) {
        return false;
    }

    pos = skipWhitespace(json, pos);
    if (pos >= json.length() || json[pos] != '{') {
        return false;
    }

    start = pos;
    int depth = 1;
    pos++;

    while (pos < json.length() && depth > 0) {
        if (json[pos] == '{') depth++;
        else if (json[pos] == '}') depth--;
        pos++;
    }

    end = pos;
    return true;
}

/**
 * @brief Get an integer value from JSON.
 */
bool getInt(const std::string& json, const std::string& key, int& value) {
    size_t pos = 0;
    if (findKey(json, pos, key)) {
        double d;
        if (parseNumber(json, pos, d)) {
            value = static_cast<int>(d);
            return true;
        }
    }
    return false;
}

/**
 * @brief Get an int64 value from JSON.
 */
bool getInt64(const std::string& json, const std::string& key, int64_t& value) {
    size_t pos = 0;
    if (findKey(json, pos, key)) {
        double d;
        if (parseNumber(json, pos, d)) {
            value = static_cast<int64_t>(d);
            return true;
        }
    }
    return false;
}

/**
 * @brief Get a float value from JSON.
 */
bool getFloat(const std::string& json, const std::string& key, float& value) {
    size_t pos = 0;
    if (findKey(json, pos, key)) {
        double d;
        if (parseNumber(json, pos, d)) {
            value = static_cast<float>(d);
            return true;
        }
    }
    return false;
}

/**
 * @brief Get a boolean value from JSON.
 */
bool getBool(const std::string& json, const std::string& key, bool& value) {
    size_t pos = 0;
    if (findKey(json, pos, key)) {
        return parseBool(json, pos, value);
    }
    return false;
}

} // anonymous namespace

// ============================================================================
// GameConfig Implementation
// ============================================================================

bool GameConfig::loadFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + filename;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
}

bool GameConfig::saveToFile(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << toJsonString();
    return file.good();
}

bool GameConfig::loadFromString(const std::string& jsonString)
{
    // Parse traffic section
    size_t start, end;
    if (findObject(jsonString, "traffic", start, end)) {
        std::string section = jsonString.substr(start, end - start);
        getInt(section, "maxSearchDistance", traffic.maxSearchDistance);
        getFloat(section, "trafficCostWeight", traffic.trafficCostWeight);
        getFloat(section, "randomizationFactor", traffic.randomizationFactor);
        getBool(section, "useAStar", traffic.useAStar);
    }

    // Parse power section
    if (findObject(jsonString, "power", start, end)) {
        std::string section = jsonString.substr(start, end - start);
        getInt(section, "coalPowerStrength", power.coalPowerStrength);
        getInt(section, "nuclearPowerStrength", power.nuclearPowerStrength);
    }

    // Parse budget section
    if (findObject(jsonString, "budget", start, end)) {
        std::string section = jsonString.substr(start, end - start);
        getInt(section, "defaultTaxRate", budget.defaultTaxRate);
        getFloat(section, "defaultRoadPercent", budget.defaultRoadPercent);
        getFloat(section, "defaultPolicePercent", budget.defaultPolicePercent);
        getFloat(section, "defaultFirePercent", budget.defaultFirePercent);
        getInt64(section, "startingFunds", budget.startingFunds);
    }

    // Parse disasters section
    if (findObject(jsonString, "disasters", start, end)) {
        std::string section = jsonString.substr(start, end - start);
        getBool(section, "enableDisasters", disasters.enableDisasters);
        getInt(section, "earthquakeFrequency", disasters.earthquakeFrequency);
        getInt(section, "fireFrequency", disasters.fireFrequency);
        getInt(section, "floodFrequency", disasters.floodFrequency);
        getInt(section, "tornadoFrequency", disasters.tornadoFrequency);
        getInt(section, "meltdownFrequency", disasters.meltdownFrequency);
    }

    // Parse world section
    if (findObject(jsonString, "world", start, end)) {
        std::string section = jsonString.substr(start, end - start);
        getInt(section, "width", world.width);
        getInt(section, "height", world.height);
    }

    // Parse simulation section
    if (findObject(jsonString, "simulation", start, end)) {
        std::string section = jsonString.substr(start, end - start);
        getInt(section, "censusFrequency", simulation.censusFrequency);
        getInt(section, "taxFrequency", simulation.taxFrequency);
        getInt(section, "evaluationFrequency", simulation.evaluationFrequency);
    }

    lastError_.clear();
    return true;
}

std::string GameConfig::toJsonString() const
{
    std::ostringstream ss;
    ss << "{\n";

    // Traffic section
    ss << "  \"traffic\": {\n";
    ss << "    \"maxSearchDistance\": " << traffic.maxSearchDistance << ",\n";
    ss << "    \"trafficCostWeight\": " << traffic.trafficCostWeight << ",\n";
    ss << "    \"randomizationFactor\": " << traffic.randomizationFactor << ",\n";
    ss << "    \"useAStar\": " << (traffic.useAStar ? "true" : "false") << "\n";
    ss << "  },\n";

    // Power section
    ss << "  \"power\": {\n";
    ss << "    \"coalPowerStrength\": " << power.coalPowerStrength << ",\n";
    ss << "    \"nuclearPowerStrength\": " << power.nuclearPowerStrength << "\n";
    ss << "  },\n";

    // Budget section
    ss << "  \"budget\": {\n";
    ss << "    \"defaultTaxRate\": " << budget.defaultTaxRate << ",\n";
    ss << "    \"defaultRoadPercent\": " << budget.defaultRoadPercent << ",\n";
    ss << "    \"defaultPolicePercent\": " << budget.defaultPolicePercent << ",\n";
    ss << "    \"defaultFirePercent\": " << budget.defaultFirePercent << ",\n";
    ss << "    \"startingFunds\": " << budget.startingFunds << "\n";
    ss << "  },\n";

    // Disasters section
    ss << "  \"disasters\": {\n";
    ss << "    \"enableDisasters\": " << (disasters.enableDisasters ? "true" : "false") << ",\n";
    ss << "    \"earthquakeFrequency\": " << disasters.earthquakeFrequency << ",\n";
    ss << "    \"fireFrequency\": " << disasters.fireFrequency << ",\n";
    ss << "    \"floodFrequency\": " << disasters.floodFrequency << ",\n";
    ss << "    \"tornadoFrequency\": " << disasters.tornadoFrequency << ",\n";
    ss << "    \"meltdownFrequency\": " << disasters.meltdownFrequency << "\n";
    ss << "  },\n";

    // World section
    ss << "  \"world\": {\n";
    ss << "    \"width\": " << world.width << ",\n";
    ss << "    \"height\": " << world.height << "\n";
    ss << "  },\n";

    // Simulation section
    ss << "  \"simulation\": {\n";
    ss << "    \"censusFrequency\": " << simulation.censusFrequency << ",\n";
    ss << "    \"taxFrequency\": " << simulation.taxFrequency << ",\n";
    ss << "    \"evaluationFrequency\": " << simulation.evaluationFrequency << "\n";
    ss << "  }\n";

    ss << "}\n";
    return ss.str();
}

void GameConfig::resetToDefaults()
{
    traffic = TrafficSettings{};
    power = PowerSettings{};
    budget = BudgetSettings{};
    disasters = DisasterSettings{};
    world = WorldSettings{};
    simulation = SimulationSettings{};
    lastError_.clear();
}

} // namespace MicropolisEngine
