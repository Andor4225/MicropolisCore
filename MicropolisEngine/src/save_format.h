/**
 * @file save_format.h
 * @brief Modern versioned save file format for Micropolis.
 *
 * MODERNIZATION (Phase 5):
 * Defines a versioned save format with:
 * - Magic number for file identification
 * - Version field for format evolution
 * - World dimensions in header (supports dynamic grid sizing)
 * - Clean separation of game state fields
 * - Optional JSON metadata
 * - Backward compatibility with legacy .cty files
 */

#ifndef MICROPOLIS_SAVE_FORMAT_H
#define MICROPOLIS_SAVE_FORMAT_H

#include <cstdint>
#include <string>

/**
 * @brief Magic number identifying Micropolis save files.
 * "MPLS" in ASCII = 0x4D504C53
 */
static constexpr uint32_t SAVE_MAGIC = 0x4D504C53;

/**
 * @brief Current save format version.
 * Increment when making breaking changes to the format.
 */
static constexpr uint32_t SAVE_VERSION_CURRENT = 1;

/**
 * @brief Minimum supported version for loading.
 */
static constexpr uint32_t SAVE_VERSION_MIN = 1;

/**
 * @brief Save file flags.
 */
enum SaveFlags : uint32_t {
    SAVE_FLAG_NONE        = 0,
    SAVE_FLAG_HAS_JSON    = 1 << 0,  ///< File contains JSON metadata section
    SAVE_FLAG_COMPRESSED  = 1 << 1,  ///< Data is compressed (future use)
    SAVE_FLAG_HAS_MOP     = 1 << 2,  ///< File contains mop (overlay) data
};

/**
 * @brief Fixed-size header for save files.
 *
 * All multi-byte values are stored in little-endian format.
 * Header size is fixed at 128 bytes for future expansion.
 */
#pragma pack(push, 1)
struct SaveFileHeader {
    // Identification (8 bytes)
    uint32_t magic;           ///< Magic number (SAVE_MAGIC)
    uint32_t version;         ///< Format version

    // File structure (8 bytes)
    uint32_t headerSize;      ///< Size of this header (for skipping)
    uint32_t flags;           ///< SaveFlags bitfield

    // World dimensions (4 bytes)
    uint16_t worldWidth;      ///< World width in tiles
    uint16_t worldHeight;     ///< World height in tiles

    // Core game state (24 bytes)
    int64_t cityTime;         ///< City time (simulation ticks)
    int64_t totalFunds;       ///< Current city funds
    int64_t cityPopulation;   ///< Last recorded population

    // Settings (8 bytes)
    uint8_t cityTax;          ///< Tax rate (0-20)
    uint8_t simSpeed;         ///< Simulation speed (0-3)
    uint8_t gameLevel;        ///< Difficulty level
    uint8_t autoBulldoze;     ///< Auto-bulldoze enabled
    uint8_t autoBudget;       ///< Auto-budget enabled
    uint8_t autoGoto;         ///< Auto-goto disasters enabled
    uint8_t enableSound;      ///< Sound enabled
    uint8_t enableDisasters;  ///< Disasters enabled

    // Funding percentages (12 bytes)
    float roadPercent;        ///< Road funding percentage
    float policePercent;      ///< Police funding percentage
    float firePercent;        ///< Fire funding percentage

    // City name (32 bytes)
    char cityName[32];        ///< Null-terminated city name

    // Data section sizes (16 bytes)
    uint32_t historyDataSize; ///< Size of history section in bytes
    uint32_t mapDataSize;     ///< Size of map section in bytes
    uint32_t mopDataSize;     ///< Size of mop section in bytes (0 if none)
    uint32_t jsonDataSize;    ///< Size of JSON section in bytes (0 if none)

    // Reserved for future use (16 bytes)
    uint8_t reserved[16];

    // Total: 128 bytes
};
#pragma pack(pop)

static_assert(sizeof(SaveFileHeader) == 128, "SaveFileHeader must be 128 bytes");

/**
 * @brief Result codes for save/load operations.
 */
enum class SaveResult {
    SUCCESS,              ///< Operation completed successfully
    FILE_NOT_FOUND,       ///< File does not exist
    FILE_OPEN_ERROR,      ///< Failed to open file
    INVALID_MAGIC,        ///< Not a Micropolis save file
    VERSION_TOO_OLD,      ///< File version too old to load
    VERSION_TOO_NEW,      ///< File version newer than supported
    INVALID_HEADER,       ///< Header data is corrupted
    INVALID_DIMENSIONS,   ///< World dimensions out of range
    READ_ERROR,           ///< Error reading file data
    WRITE_ERROR,          ///< Error writing file data
    DIMENSION_MISMATCH,   ///< File dimensions don't match current world
    LEGACY_FORMAT,        ///< File is in legacy format (load succeeded)
};

/**
 * @brief Convert SaveResult to human-readable string.
 */
inline const char* saveResultToString(SaveResult result) {
    switch (result) {
        case SaveResult::SUCCESS:           return "Success";
        case SaveResult::FILE_NOT_FOUND:    return "File not found";
        case SaveResult::FILE_OPEN_ERROR:   return "Failed to open file";
        case SaveResult::INVALID_MAGIC:     return "Not a valid save file";
        case SaveResult::VERSION_TOO_OLD:   return "Save file version too old";
        case SaveResult::VERSION_TOO_NEW:   return "Save file version too new";
        case SaveResult::INVALID_HEADER:    return "Corrupted file header";
        case SaveResult::INVALID_DIMENSIONS: return "Invalid world dimensions";
        case SaveResult::READ_ERROR:        return "Error reading file";
        case SaveResult::WRITE_ERROR:       return "Error writing file";
        case SaveResult::DIMENSION_MISMATCH: return "World dimensions don't match";
        case SaveResult::LEGACY_FORMAT:     return "Legacy format loaded";
        default:                            return "Unknown error";
    }
}

/**
 * @brief Metadata that can be stored in the optional JSON section.
 */
struct SaveMetadata {
    std::string createdBy;        ///< Application that created the save
    std::string createdVersion;   ///< Version of the application
    int64_t createdTimestamp;     ///< Unix timestamp of creation
    int64_t modifiedTimestamp;    ///< Unix timestamp of last modification
    std::string description;      ///< User-provided description

    SaveMetadata() : createdTimestamp(0), modifiedTimestamp(0) {}
};

#endif // MICROPOLIS_SAVE_FORMAT_H
