/**
 * @file ScenarioConfig.cpp
 * @brief Implementation of scenario configuration and loading system.
 *
 * MODERNIZATION (Phase 6.5):
 * Simple JSON parsing without external dependencies.
 */

#include "ScenarioConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace MicropolisEngine {

// Simple JSON value types for basic parsing
namespace {

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::string extractString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";

    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";

    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";

    return json.substr(pos + 1, end - pos - 1);
}

int64_t extractInt(const std::string& json, const std::string& key, int64_t defaultValue = 0) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultValue;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultValue;

    pos++;
    while (pos < json.size() && std::isspace(json[pos])) pos++;

    std::string numStr;
    bool negative = false;
    if (pos < json.size() && json[pos] == '-') {
        negative = true;
        pos++;
    }
    while (pos < json.size() && std::isdigit(json[pos])) {
        numStr += json[pos++];
    }

    if (numStr.empty()) return defaultValue;

    int64_t value = std::stoll(numStr);
    return negative ? -value : value;
}

float extractFloat(const std::string& json, const std::string& key, float defaultValue = 0.0f) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultValue;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultValue;

    pos++;
    while (pos < json.size() && std::isspace(json[pos])) pos++;

    std::string numStr;
    while (pos < json.size() && (std::isdigit(json[pos]) || json[pos] == '.' || json[pos] == '-')) {
        numStr += json[pos++];
    }

    if (numStr.empty()) return defaultValue;

    return std::stof(numStr);
}

bool extractBool(const std::string& json, const std::string& key, bool defaultValue = false) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultValue;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultValue;

    pos++;
    while (pos < json.size() && std::isspace(json[pos])) pos++;

    if (pos + 4 <= json.size() && json.substr(pos, 4) == "true") return true;
    if (pos + 5 <= json.size() && json.substr(pos, 5) == "false") return false;

    return defaultValue;
}

std::string extractObject(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos = json.find('{', pos);
    if (pos == std::string::npos) return "";

    int depth = 1;
    size_t start = pos;
    pos++;

    while (pos < json.size() && depth > 0) {
        if (json[pos] == '{') depth++;
        else if (json[pos] == '}') depth--;
        pos++;
    }

    return json.substr(start, pos - start);
}

std::string extractArray(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos = json.find('[', pos);
    if (pos == std::string::npos) return "";

    int depth = 1;
    size_t start = pos;
    pos++;

    while (pos < json.size() && depth > 0) {
        if (json[pos] == '[') depth++;
        else if (json[pos] == ']') depth--;
        pos++;
    }

    return json.substr(start, pos - start);
}

std::vector<std::string> splitArrayElements(const std::string& arrayJson) {
    std::vector<std::string> elements;
    if (arrayJson.size() < 2) return elements;

    // Remove brackets
    std::string content = arrayJson.substr(1, arrayJson.size() - 2);

    int depth = 0;
    size_t start = 0;

    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];
        if (c == '{' || c == '[') depth++;
        else if (c == '}' || c == ']') depth--;
        else if (c == ',' && depth == 0) {
            elements.push_back(trim(content.substr(start, i - start)));
            start = i + 1;
        }
    }

    std::string last = trim(content.substr(start));
    if (!last.empty()) {
        elements.push_back(last);
    }

    return elements;
}

ScenarioDisaster parseDisaster(const std::string& str) {
    if (str == "fire") return ScenarioDisaster::Fire;
    if (str == "flood") return ScenarioDisaster::Flood;
    if (str == "tornado") return ScenarioDisaster::Tornado;
    if (str == "earthquake") return ScenarioDisaster::Earthquake;
    if (str == "monster") return ScenarioDisaster::Monster;
    if (str == "meltdown") return ScenarioDisaster::Meltdown;
    return ScenarioDisaster::None;
}

std::string disasterToString(ScenarioDisaster d) {
    switch (d) {
        case ScenarioDisaster::Fire: return "fire";
        case ScenarioDisaster::Flood: return "flood";
        case ScenarioDisaster::Tornado: return "tornado";
        case ScenarioDisaster::Earthquake: return "earthquake";
        case ScenarioDisaster::Monster: return "monster";
        case ScenarioDisaster::Meltdown: return "meltdown";
        default: return "none";
    }
}

VictoryType parseVictoryType(const std::string& str) {
    if (str == "population") return VictoryType::Population;
    if (str == "score") return VictoryType::Score;
    if (str == "funds") return VictoryType::Funds;
    if (str == "survive") return VictoryType::SurviveTime;
    return VictoryType::Custom;
}

std::string victoryTypeToString(VictoryType v) {
    switch (v) {
        case VictoryType::Population: return "population";
        case VictoryType::Score: return "score";
        case VictoryType::Funds: return "funds";
        case VictoryType::SurviveTime: return "survive";
        default: return "custom";
    }
}

} // anonymous namespace

// ScenarioLoader implementation

ScenarioLoadResult ScenarioLoader::loadFromFile(
    const std::string& filePath,
    ScenarioConfig& config)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return ScenarioLoadResult::FileNotFound;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str(), config);
}

ScenarioLoadResult ScenarioLoader::loadFromString(
    const std::string& jsonContent,
    ScenarioConfig& config)
{
    // Reset config
    config = ScenarioConfig();

    // Parse metadata
    config.id = extractString(jsonContent, "id");
    config.name = extractString(jsonContent, "name");
    config.description = extractString(jsonContent, "description");
    config.author = extractString(jsonContent, "author");
    config.version = extractString(jsonContent, "version");
    config.difficulty = static_cast<int>(extractInt(jsonContent, "difficulty", 1));

    if (config.id.empty() || config.name.empty()) {
        return ScenarioLoadResult::MissingRequiredField;
    }

    // Parse starting config
    std::string startingJson = extractObject(jsonContent, "starting");
    if (!startingJson.empty()) {
        config.starting.funds = extractInt(startingJson, "funds", 20000);
        config.starting.taxRate = static_cast<int>(extractInt(startingJson, "taxRate", 7));
        config.starting.worldWidth = static_cast<int>(extractInt(startingJson, "worldWidth", 120));
        config.starting.worldHeight = static_cast<int>(extractInt(startingJson, "worldHeight", 100));
        config.starting.seed = extractInt(startingJson, "seed", 0);
        config.starting.generateCity = extractBool(startingJson, "generateCity", false);
        config.starting.loadFile = extractString(startingJson, "loadFile");
    }

    // Parse rules
    std::string rulesJson = extractObject(jsonContent, "rules");
    if (!rulesJson.empty()) {
        config.rules.disastersEnabled = extractBool(rulesJson, "disastersEnabled", true);
        config.rules.autoGoto = extractBool(rulesJson, "autoGoto", true);
        config.rules.autoBudget = extractBool(rulesJson, "autoBudget", false);
        config.rules.pollutionMultiplier = extractFloat(rulesJson, "pollutionMultiplier", 1.0f);
        config.rules.crimeMultiplier = extractFloat(rulesJson, "crimeMultiplier", 1.0f);
        config.rules.trafficMultiplier = extractFloat(rulesJson, "trafficMultiplier", 1.0f);
        config.rules.growthMultiplier = extractFloat(rulesJson, "growthMultiplier", 1.0f);
        config.rules.maxGameSpeed = static_cast<int>(extractInt(rulesJson, "maxGameSpeed", 3));
    }

    // Parse victory conditions
    std::string victoriesJson = extractArray(jsonContent, "victories");
    if (!victoriesJson.empty()) {
        auto elements = splitArrayElements(victoriesJson);
        for (const auto& elem : elements) {
            VictoryCondition vc;
            vc.type = parseVictoryType(extractString(elem, "type"));
            vc.targetValue = extractInt(elem, "target", 0);
            vc.timeLimit = static_cast<int>(extractInt(elem, "timeLimit", 0));
            vc.description = extractString(elem, "description");
            config.victories.push_back(vc);
        }
    }

    if (config.victories.empty()) {
        return ScenarioLoadResult::MissingRequiredField;
    }

    // Parse events
    std::string eventsJson = extractArray(jsonContent, "events");
    if (!eventsJson.empty()) {
        auto elements = splitArrayElements(eventsJson);
        for (const auto& elem : elements) {
            ScenarioEvent evt;
            evt.triggerTime = static_cast<int>(extractInt(elem, "time", 0));
            evt.disaster = parseDisaster(extractString(elem, "disaster"));
            evt.locationX = static_cast<int>(extractInt(elem, "x", -1));
            evt.locationY = static_cast<int>(extractInt(elem, "y", -1));
            evt.message = extractString(elem, "message");
            config.events.push_back(evt);
        }
    }

    // Sort events by trigger time
    std::sort(config.events.begin(), config.events.end(),
        [](const ScenarioEvent& a, const ScenarioEvent& b) {
            return a.triggerTime < b.triggerTime;
        });

    return ScenarioLoadResult::Success;
}

bool ScenarioLoader::saveToFile(
    const std::string& filePath,
    const ScenarioConfig& config)
{
    std::string json = exportToString(config);

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << json;
    return true;
}

std::string ScenarioLoader::exportToString(const ScenarioConfig& config)
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"id\": \"" << config.id << "\",\n";
    ss << "  \"name\": \"" << config.name << "\",\n";
    ss << "  \"description\": \"" << config.description << "\",\n";
    ss << "  \"author\": \"" << config.author << "\",\n";
    ss << "  \"version\": \"" << config.version << "\",\n";
    ss << "  \"difficulty\": " << config.difficulty << ",\n";

    // Starting config
    ss << "  \"starting\": {\n";
    ss << "    \"funds\": " << config.starting.funds << ",\n";
    ss << "    \"taxRate\": " << config.starting.taxRate << ",\n";
    ss << "    \"worldWidth\": " << config.starting.worldWidth << ",\n";
    ss << "    \"worldHeight\": " << config.starting.worldHeight << ",\n";
    ss << "    \"seed\": " << config.starting.seed << ",\n";
    ss << "    \"generateCity\": " << (config.starting.generateCity ? "true" : "false") << ",\n";
    ss << "    \"loadFile\": \"" << config.starting.loadFile << "\"\n";
    ss << "  },\n";

    // Rules
    ss << "  \"rules\": {\n";
    ss << "    \"disastersEnabled\": " << (config.rules.disastersEnabled ? "true" : "false") << ",\n";
    ss << "    \"autoGoto\": " << (config.rules.autoGoto ? "true" : "false") << ",\n";
    ss << "    \"autoBudget\": " << (config.rules.autoBudget ? "true" : "false") << ",\n";
    ss << "    \"pollutionMultiplier\": " << config.rules.pollutionMultiplier << ",\n";
    ss << "    \"crimeMultiplier\": " << config.rules.crimeMultiplier << ",\n";
    ss << "    \"trafficMultiplier\": " << config.rules.trafficMultiplier << ",\n";
    ss << "    \"growthMultiplier\": " << config.rules.growthMultiplier << ",\n";
    ss << "    \"maxGameSpeed\": " << config.rules.maxGameSpeed << "\n";
    ss << "  },\n";

    // Victories
    ss << "  \"victories\": [\n";
    for (size_t i = 0; i < config.victories.size(); ++i) {
        const auto& v = config.victories[i];
        ss << "    {\n";
        ss << "      \"type\": \"" << victoryTypeToString(v.type) << "\",\n";
        ss << "      \"target\": " << v.targetValue << ",\n";
        ss << "      \"timeLimit\": " << v.timeLimit << ",\n";
        ss << "      \"description\": \"" << v.description << "\"\n";
        ss << "    }" << (i < config.victories.size() - 1 ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // Events
    ss << "  \"events\": [\n";
    for (size_t i = 0; i < config.events.size(); ++i) {
        const auto& e = config.events[i];
        ss << "    {\n";
        ss << "      \"time\": " << e.triggerTime << ",\n";
        ss << "      \"disaster\": \"" << disasterToString(e.disaster) << "\",\n";
        ss << "      \"x\": " << e.locationX << ",\n";
        ss << "      \"y\": " << e.locationY << ",\n";
        ss << "      \"message\": \"" << e.message << "\"\n";
        ss << "    }" << (i < config.events.size() - 1 ? "," : "") << "\n";
    }
    ss << "  ]\n";

    ss << "}\n";
    return ss.str();
}

std::vector<std::string> ScenarioLoader::listScenarios(const std::string& directory)
{
    std::vector<std::string> scenarios;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".json" || ext == ".scenario") {
                    scenarios.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Directory doesn't exist or isn't accessible
    }

    return scenarios;
}

std::string ScenarioLoader::getErrorMessage(ScenarioLoadResult result)
{
    switch (result) {
        case ScenarioLoadResult::Success:
            return "Success";
        case ScenarioLoadResult::FileNotFound:
            return "Scenario file not found";
        case ScenarioLoadResult::ParseError:
            return "Failed to parse JSON";
        case ScenarioLoadResult::InvalidFormat:
            return "Invalid scenario format";
        case ScenarioLoadResult::MissingRequiredField:
            return "Missing required field (id, name, or victories)";
        case ScenarioLoadResult::VersionMismatch:
            return "Incompatible scenario version";
        default:
            return "Unknown error";
    }
}

// ScenarioRunner implementation

ScenarioRunner::ScenarioRunner() = default;

void ScenarioRunner::initialize(const ScenarioConfig& config)
{
    config_ = config;
    active_ = true;
    won_ = false;
    lost_ = false;
    progress_ = 0.0f;
    nextEventIndex_ = 0;
    pendingEvents_.clear();
}

void ScenarioRunner::update(int64_t cityTime, int64_t population, int score, int64_t funds)
{
    if (!active_ || won_ || lost_) {
        return;
    }

    // Check for pending events
    while (nextEventIndex_ < config_.events.size() &&
           config_.events[nextEventIndex_].triggerTime <= cityTime) {
        pendingEvents_.push_back(config_.events[nextEventIndex_]);
        nextEventIndex_++;
    }

    // Check victory conditions
    for (const auto& vc : config_.victories) {
        bool conditionMet = false;

        switch (vc.type) {
            case VictoryType::Population:
                conditionMet = population >= vc.targetValue;
                if (!conditionMet && vc.targetValue > 0) {
                    progress_ = static_cast<float>(population) / vc.targetValue;
                }
                break;

            case VictoryType::Score:
                conditionMet = score >= vc.targetValue;
                if (!conditionMet && vc.targetValue > 0) {
                    progress_ = static_cast<float>(score) / vc.targetValue;
                }
                break;

            case VictoryType::Funds:
                conditionMet = funds >= vc.targetValue;
                if (!conditionMet && vc.targetValue > 0) {
                    progress_ = static_cast<float>(funds) / vc.targetValue;
                }
                break;

            case VictoryType::SurviveTime:
                conditionMet = cityTime >= vc.targetValue;
                if (!conditionMet && vc.targetValue > 0) {
                    progress_ = static_cast<float>(cityTime) / vc.targetValue;
                }
                break;

            case VictoryType::Custom:
                // Custom conditions must be checked externally
                break;
        }

        if (conditionMet) {
            won_ = true;
            progress_ = 1.0f;
            return;
        }

        // Check time limit
        if (vc.timeLimit > 0 && cityTime >= vc.timeLimit) {
            lost_ = true;
            return;
        }
    }

    // Clamp progress
    if (progress_ > 1.0f) progress_ = 1.0f;
    if (progress_ < 0.0f) progress_ = 0.0f;
}

std::vector<ScenarioEvent> ScenarioRunner::getPendingEvents()
{
    std::vector<ScenarioEvent> events = std::move(pendingEvents_);
    pendingEvents_.clear();
    return events;
}

int ScenarioRunner::getRemainingTime() const
{
    if (config_.victories.empty()) {
        return -1;
    }

    for (const auto& vc : config_.victories) {
        if (vc.timeLimit > 0) {
            return vc.timeLimit; // Would need current time to calculate remaining
        }
    }

    return -1;
}

void ScenarioRunner::reset()
{
    active_ = false;
    won_ = false;
    lost_ = false;
    progress_ = 0.0f;
    nextEventIndex_ = 0;
    pendingEvents_.clear();
}

} // namespace MicropolisEngine
