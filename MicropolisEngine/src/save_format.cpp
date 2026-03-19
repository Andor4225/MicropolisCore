/**
 * @file save_format.cpp
 * @brief Modern versioned save file implementation for Micropolis.
 *
 * MODERNIZATION (Phase 5):
 * Implements versioned save/load with backward compatibility.
 */

#include "save_format.h"
#include "micropolis.h"
#include "world_dimensions.h"
#include <fstream>
#include <cstring>
#include <ctime>
#include <sstream>

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

/**
 * @brief Check if file appears to be in legacy format.
 *
 * Legacy files have specific sizes based on 120x100 world.
 */
bool isLegacyFormat(std::ifstream& file) {
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Legacy format sizes for 120x100 world
    const int historySize = (6 * HISTORY_LENGTH) + MISC_HISTORY_LENGTH;
    const int mapSize = 120 * 100 * sizeof(short);
    const int legacyMapOnly = historySize + mapSize;      // 27120
    const int legacyWithMop = historySize + mapSize * 2;  // 51120

    return (size == legacyMapOnly || size == legacyWithMop);
}

/**
 * @brief Check if file has modern format magic number.
 */
bool hasModernMagic(std::ifstream& file) {
    uint32_t magic = 0;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.seekg(0, std::ios::beg);
    return (magic == SAVE_MAGIC);
}

/**
 * @brief Write data in little-endian format.
 */
template<typename T>
bool writeLE(std::ofstream& file, T value) {
    // On little-endian systems (x86/x64), we can write directly
    // For portability, we should byte-swap on big-endian systems
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
    return file.good();
}

/**
 * @brief Read data in little-endian format.
 */
template<typename T>
bool readLE(std::ifstream& file, T& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return file.good();
}

} // anonymous namespace

// ============================================================================
// Micropolis Save/Load Methods (Modern Format)
// ============================================================================

/**
 * Save game in modern versioned format.
 */
SaveResult Micropolis::saveFileModern(const std::string& filename, bool includeJson)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return SaveResult::FILE_OPEN_ERROR;
    }

    // Prepare header
    SaveFileHeader header = {};
    header.magic = SAVE_MAGIC;
    header.version = SAVE_VERSION_CURRENT;
    header.headerSize = sizeof(SaveFileHeader);
    header.flags = SAVE_FLAG_HAS_MOP;
    if (includeJson) {
        header.flags |= SAVE_FLAG_HAS_JSON;
    }

    // World dimensions
    header.worldWidth = static_cast<uint16_t>(worldWidth());
    header.worldHeight = static_cast<uint16_t>(worldHeight());

    // Core game state
    header.cityTime = cityTime;
    header.totalFunds = totalFunds;
    header.cityPopulation = cityPop;

    // Settings
    header.cityTax = static_cast<uint8_t>(cityTax);
    header.simSpeed = static_cast<uint8_t>(simSpeed);
    header.gameLevel = static_cast<uint8_t>(gameLevel);
    header.autoBulldoze = autoBulldoze ? 1 : 0;
    header.autoBudget = autoBudget ? 1 : 0;
    header.autoGoto = autoGoto ? 1 : 0;
    header.enableSound = enableSound ? 1 : 0;
    header.enableDisasters = enableDisasters ? 1 : 0;

    // Funding percentages
    header.roadPercent = roadPercent;
    header.policePercent = policePercent;
    header.firePercent = firePercent;

    // City name (truncate if necessary)
    strncpy(header.cityName, cityName.c_str(), sizeof(header.cityName) - 1);
    header.cityName[sizeof(header.cityName) - 1] = '\0';

    // Calculate data section sizes
    const size_t historyElements = HISTORY_LENGTH / sizeof(short);
    const size_t miscHistoryElements = MISC_HISTORY_LENGTH / sizeof(short);
    header.historyDataSize = static_cast<uint32_t>(
        (6 * historyElements + miscHistoryElements) * sizeof(short));
    header.mapDataSize = static_cast<uint32_t>(
        header.worldWidth * header.worldHeight * sizeof(unsigned short));
    header.mopDataSize = header.mapDataSize;

    // Build JSON metadata if requested
    std::string jsonData;
    if (includeJson) {
        jsonData = buildSaveMetadataJson();
        header.jsonDataSize = static_cast<uint32_t>(jsonData.size());
    } else {
        header.jsonDataSize = 0;
    }

    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!file.good()) {
        return SaveResult::WRITE_ERROR;
    }

    // Write history data
    file.write(reinterpret_cast<const char*>(resHist.data()),
               historyElements * sizeof(short));
    file.write(reinterpret_cast<const char*>(comHist.data()),
               historyElements * sizeof(short));
    file.write(reinterpret_cast<const char*>(indHist.data()),
               historyElements * sizeof(short));
    file.write(reinterpret_cast<const char*>(crimeHist.data()),
               historyElements * sizeof(short));
    file.write(reinterpret_cast<const char*>(pollutionHist.data()),
               historyElements * sizeof(short));
    file.write(reinterpret_cast<const char*>(moneyHist.data()),
               historyElements * sizeof(short));
    file.write(reinterpret_cast<const char*>(miscHist.data()),
               miscHistoryElements * sizeof(short));

    if (!file.good()) {
        return SaveResult::WRITE_ERROR;
    }

    // Write map data
    file.write(reinterpret_cast<const char*>(mapBaseStorage.data()),
               header.mapDataSize);
    if (!file.good()) {
        return SaveResult::WRITE_ERROR;
    }

    // Write mop data
    file.write(reinterpret_cast<const char*>(mopBaseStorage.data()),
               header.mopDataSize);
    if (!file.good()) {
        return SaveResult::WRITE_ERROR;
    }

    // Write JSON metadata if present
    if (includeJson && !jsonData.empty()) {
        file.write(jsonData.data(), jsonData.size());
        if (!file.good()) {
            return SaveResult::WRITE_ERROR;
        }
    }

    return SaveResult::SUCCESS;
}

/**
 * Load game from modern versioned format.
 */
SaveResult Micropolis::loadFileModern(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return SaveResult::FILE_NOT_FOUND;
    }

    // Check for legacy format first
    if (!hasModernMagic(file)) {
        if (isLegacyFormat(file)) {
            file.close();
            // Delegate to legacy loader
            if (loadFileData(filename)) {
                return SaveResult::LEGACY_FORMAT;
            }
            return SaveResult::READ_ERROR;
        }
        return SaveResult::INVALID_MAGIC;
    }

    // Read header
    SaveFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file.good()) {
        return SaveResult::READ_ERROR;
    }

    // Validate header
    if (header.magic != SAVE_MAGIC) {
        return SaveResult::INVALID_MAGIC;
    }

    if (header.version < SAVE_VERSION_MIN) {
        return SaveResult::VERSION_TOO_OLD;
    }

    if (header.version > SAVE_VERSION_CURRENT) {
        return SaveResult::VERSION_TOO_NEW;
    }

    // Validate dimensions
    if (header.worldWidth < WorldDimensions::MIN_WIDTH ||
        header.worldWidth > WorldDimensions::MAX_WIDTH ||
        header.worldHeight < WorldDimensions::MIN_HEIGHT ||
        header.worldHeight > WorldDimensions::MAX_HEIGHT) {
        return SaveResult::INVALID_DIMENSIONS;
    }

    // Check if dimensions match current world
    if (header.worldWidth != worldWidth() ||
        header.worldHeight != worldHeight()) {
        // For now, require dimensions to match
        // Future: could resize world to match save file
        return SaveResult::DIMENSION_MISMATCH;
    }

    // Skip to data section (in case header is larger than expected)
    file.seekg(header.headerSize, std::ios::beg);

    // Read history data
    const size_t historyElements = HISTORY_LENGTH / sizeof(short);
    const size_t miscHistoryElements = MISC_HISTORY_LENGTH / sizeof(short);

    file.read(reinterpret_cast<char*>(resHist.data()),
              historyElements * sizeof(short));
    file.read(reinterpret_cast<char*>(comHist.data()),
              historyElements * sizeof(short));
    file.read(reinterpret_cast<char*>(indHist.data()),
              historyElements * sizeof(short));
    file.read(reinterpret_cast<char*>(crimeHist.data()),
              historyElements * sizeof(short));
    file.read(reinterpret_cast<char*>(pollutionHist.data()),
              historyElements * sizeof(short));
    file.read(reinterpret_cast<char*>(moneyHist.data()),
              historyElements * sizeof(short));
    file.read(reinterpret_cast<char*>(miscHist.data()),
              miscHistoryElements * sizeof(short));

    if (!file.good()) {
        return SaveResult::READ_ERROR;
    }

    // Read map data
    file.read(reinterpret_cast<char*>(mapBaseStorage.data()),
              header.mapDataSize);
    if (!file.good()) {
        return SaveResult::READ_ERROR;
    }

    // Read mop data if present
    if (header.flags & SAVE_FLAG_HAS_MOP) {
        file.read(reinterpret_cast<char*>(mopBaseStorage.data()),
                  header.mopDataSize);
        if (!file.good()) {
            return SaveResult::READ_ERROR;
        }
    } else {
        std::fill(mopBaseStorage.begin(), mopBaseStorage.end(), 0);
    }

    // Restore game state from header
    cityTime = header.cityTime;
    setFunds(header.totalFunds);
    cityPop = header.cityPopulation;

    setCityTax(header.cityTax);
    setSpeed(header.simSpeed);
    setGameLevel(static_cast<GameLevel>(header.gameLevel));
    setAutoBulldoze(header.autoBulldoze != 0);
    setAutoBudget(header.autoBudget != 0);
    setAutoGoto(header.autoGoto != 0);
    setEnableSound(header.enableSound != 0);
    enableDisasters = (header.enableDisasters != 0);

    roadPercent = header.roadPercent;
    policePercent = header.policePercent;
    firePercent = header.firePercent;

    if (header.cityName[0] != '\0') {
        setCityName(std::string(header.cityName));
    }

    // Read JSON metadata if present (for future use)
    if ((header.flags & SAVE_FLAG_HAS_JSON) && header.jsonDataSize > 0) {
        std::string jsonData(header.jsonDataSize, '\0');
        file.read(&jsonData[0], header.jsonDataSize);
        // Could parse and use metadata here
    }

    // Finalize load
    mustUpdateOptions = true;
    changeCensus();
    initFundingLevel();

    return SaveResult::SUCCESS;
}

/**
 * Build JSON metadata string for save file.
 */
std::string Micropolis::buildSaveMetadataJson() const
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"format\": \"Micropolis Save\",\n";
    ss << "  \"version\": " << SAVE_VERSION_CURRENT << ",\n";
    ss << "  \"engine\": \"MicropolisCore\",\n";
    ss << "  \"timestamp\": " << std::time(nullptr) << ",\n";
    ss << "  \"world\": {\n";
    ss << "    \"width\": " << worldWidth() << ",\n";
    ss << "    \"height\": " << worldHeight() << "\n";
    ss << "  },\n";
    ss << "  \"city\": {\n";
    ss << "    \"name\": \"" << cityName << "\",\n";
    ss << "    \"population\": " << cityPop << ",\n";
    ss << "    \"funds\": " << totalFunds << ",\n";
    ss << "    \"time\": " << cityTime << "\n";
    ss << "  }\n";
    ss << "}\n";
    return ss.str();
}

/**
 * Export full game state to JSON format.
 */
bool Micropolis::exportToJson(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "{\n";
    file << "  \"format\": \"Micropolis JSON Export\",\n";
    file << "  \"version\": 1,\n";
    file << "  \"timestamp\": " << std::time(nullptr) << ",\n\n";

    // World info
    file << "  \"world\": {\n";
    file << "    \"width\": " << worldWidth() << ",\n";
    file << "    \"height\": " << worldHeight() << "\n";
    file << "  },\n\n";

    // City state
    file << "  \"city\": {\n";
    file << "    \"name\": \"" << cityName << "\",\n";
    file << "    \"time\": " << cityTime << ",\n";
    file << "    \"funds\": " << totalFunds << ",\n";
    file << "    \"population\": " << cityPop << ",\n";
    file << "    \"score\": " << cityScore << ",\n";
    file << "    \"class\": " << static_cast<int>(cityClass) << "\n";
    file << "  },\n\n";

    // Settings
    file << "  \"settings\": {\n";
    file << "    \"taxRate\": " << cityTax << ",\n";
    file << "    \"speed\": " << simSpeed << ",\n";
    file << "    \"gameLevel\": " << static_cast<int>(gameLevel) << ",\n";
    file << "    \"autoBulldoze\": " << (autoBulldoze ? "true" : "false") << ",\n";
    file << "    \"autoBudget\": " << (autoBudget ? "true" : "false") << ",\n";
    file << "    \"autoGoto\": " << (autoGoto ? "true" : "false") << ",\n";
    file << "    \"enableSound\": " << (enableSound ? "true" : "false") << ",\n";
    file << "    \"enableDisasters\": " << (enableDisasters ? "true" : "false") << "\n";
    file << "  },\n\n";

    // Funding
    file << "  \"funding\": {\n";
    file << "    \"roadPercent\": " << roadPercent << ",\n";
    file << "    \"policePercent\": " << policePercent << ",\n";
    file << "    \"firePercent\": " << firePercent << "\n";
    file << "  },\n\n";

    // Statistics
    file << "  \"statistics\": {\n";
    file << "    \"residential\": " << resPop << ",\n";
    file << "    \"commercial\": " << comPop << ",\n";
    file << "    \"industrial\": " << indPop << ",\n";
    file << "    \"totalPop\": " << totalPop << ",\n";
    file << "    \"totalZonePop\": " << totalZonePop << ",\n";
    file << "    \"crimeAverage\": " << crimeAverage << ",\n";
    file << "    \"pollutionAverage\": " << pollutionAverage << ",\n";
    file << "    \"landValueAverage\": " << landValueAverage << "\n";
    file << "  },\n\n";

    // Note about map data
    file << "  \"_note\": \"Map tile data not included in JSON export for readability. Use binary format for full saves.\"\n";

    file << "}\n";

    return file.good();
}

/**
 * Detect save file format.
 */
SaveResult Micropolis::detectSaveFormat(const std::string& filename, bool& isModern, bool& isLegacy)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return SaveResult::FILE_NOT_FOUND;
    }

    isModern = hasModernMagic(file);
    isLegacy = !isModern && isLegacyFormat(file);

    if (!isModern && !isLegacy) {
        return SaveResult::INVALID_MAGIC;
    }

    return SaveResult::SUCCESS;
}
