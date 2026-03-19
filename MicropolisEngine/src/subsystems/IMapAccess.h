/**
 * @file IMapAccess.h
 * @brief Interface for accessing game map and density maps.
 *
 * MODERNIZATION (Phase 3):
 * This interface abstracts map access for subsystems, enabling:
 * - Unit testing with mock maps
 * - Potential for different map implementations
 * - Clear documentation of subsystem map dependencies
 */

#ifndef MICROPOLIS_IMAP_ACCESS_H
#define MICROPOLIS_IMAP_ACCESS_H

#include "../data_types.h"
#include "../map_type.h"

namespace MicropolisEngine {

/**
 * @brief Read-only interface for accessing map data.
 *
 * Subsystems that only need to read map data should use this interface.
 */
class IMapReader {
public:
    virtual ~IMapReader() = default;

    // World map access
    virtual MapTile getTile(int x, int y) const = 0;
    virtual bool isOnMap(int x, int y) const = 0;

    // Tile flag checks
    virtual bool hasPowerBit(int x, int y) const = 0;
    virtual bool hasCondBit(int x, int y) const = 0;
    virtual bool hasBurnBit(int x, int y) const = 0;
    virtual bool hasBullBit(int x, int y) const = 0;
    virtual bool hasAnimBit(int x, int y) const = 0;
    virtual bool hasZoneBit(int x, int y) const = 0;

    // Density map access (read-only)
    virtual int getPopulationDensity(int x, int y) const = 0;
    virtual int getTrafficDensity(int x, int y) const = 0;
    virtual int getPollutionDensity(int x, int y) const = 0;
    virtual int getLandValue(int x, int y) const = 0;
    virtual int getCrimeRate(int x, int y) const = 0;

    // Power grid access (read-only)
    virtual bool isPowered(int x, int y) const = 0;

    // Map dimensions
    virtual int getWorldWidth() const = 0;
    virtual int getWorldHeight() const = 0;
};

/**
 * @brief Full interface for reading and writing map data.
 *
 * Subsystems that need to modify map data should use this interface.
 */
class IMapAccess : public IMapReader {
public:
    // World map modification
    virtual void setTile(int x, int y, MapTile tile) = 0;
    virtual void setTileFlags(int x, int y, MapTile flags) = 0;
    virtual void clearTileFlags(int x, int y, MapTile flags) = 0;

    // Density map modification
    virtual void setPopulationDensity(int x, int y, int value) = 0;
    virtual void setTrafficDensity(int x, int y, int value) = 0;
    virtual void setPollutionDensity(int x, int y, int value) = 0;
    virtual void setLandValue(int x, int y, int value) = 0;
    virtual void setCrimeRate(int x, int y, int value) = 0;

    // Power grid modification
    virtual void setPowered(int x, int y, bool powered) = 0;

    // Bulk operations
    virtual void clearPowerGrid() = 0;
    virtual void decayTrafficDensity() = 0;
};

/**
 * @brief Tile flag constants.
 *
 * These match the original game's tile flag bits.
 */
namespace TileFlags {
    constexpr MapTile PWRBIT  = 0x8000;  ///< Power bit
    constexpr MapTile CONDBIT = 0x4000;  ///< Conducts electricity
    constexpr MapTile BURNBIT = 0x2000;  ///< Can burn
    constexpr MapTile BULLBIT = 0x1000;  ///< Can be bulldozed
    constexpr MapTile ANIMBIT = 0x0800;  ///< Is animated
    constexpr MapTile ZONEBIT = 0x0400;  ///< Is a zone center
    constexpr MapTile ALLBITS = 0xFC00;  ///< All flag bits
    constexpr MapTile LOMASK  = 0x03FF;  ///< Tile number mask
}

} // namespace MicropolisEngine

#endif // MICROPOLIS_IMAP_ACCESS_H
