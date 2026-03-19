/**
 * @file tile_flags.h
 * @brief Type-safe tile flag manipulation utilities.
 *
 * MODERNIZATION (Phase 6.2):
 * Provides type-safe wrappers for tile flag manipulation.
 * Works alongside existing MapTileBits for backward compatibility.
 */

#ifndef MICROPOLIS_TILE_FLAGS_H
#define MICROPOLIS_TILE_FLAGS_H

#include <cstdint>
#include <type_traits>

/**
 * @brief Type-safe tile flag enumeration.
 *
 * Use TileFlags for new code instead of raw MapTileBits.
 * Provides better type safety and clearer intent.
 */
enum class TileFlag : uint16_t {
    None     = 0,
    Zone     = 0x0400,  ///< Tile is center of a zone
    Animated = 0x0800,  ///< Tile is animated
    Bulldozable = 0x1000,  ///< Tile can be bulldozed
    Burnable = 0x2000,  ///< Tile can catch fire
    Conductive = 0x4000,  ///< Tile conducts electricity
    Powered  = 0x8000,  ///< Tile has power
};

/**
 * @brief Enable bitwise operations on TileFlag.
 */
constexpr TileFlag operator|(TileFlag a, TileFlag b) {
    return static_cast<TileFlag>(
        static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

constexpr TileFlag operator&(TileFlag a, TileFlag b) {
    return static_cast<TileFlag>(
        static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

constexpr TileFlag operator~(TileFlag a) {
    return static_cast<TileFlag>(~static_cast<uint16_t>(a));
}

constexpr TileFlag& operator|=(TileFlag& a, TileFlag b) {
    return a = a | b;
}

constexpr TileFlag& operator&=(TileFlag& a, TileFlag b) {
    return a = a & b;
}

/**
 * @brief Check if a flag is set.
 */
constexpr bool hasFlag(TileFlag flags, TileFlag test) {
    return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(test)) != 0;
}

/**
 * @brief Type-safe tile value wrapper.
 *
 * Encapsulates a map tile value and provides safe access to its components.
 */
class TileValue {
public:
    static constexpr uint16_t TILE_MASK = 0x03FF;
    static constexpr uint16_t FLAG_MASK = 0xFC00;

    constexpr TileValue() : value_(0) {}
    constexpr explicit TileValue(uint16_t raw) : value_(raw) {}
    constexpr TileValue(uint16_t tile, TileFlag flags)
        : value_((tile & TILE_MASK) | static_cast<uint16_t>(flags)) {}

    /** Get the tile number (0-1023). */
    constexpr uint16_t tile() const { return value_ & TILE_MASK; }

    /** Get all flags. */
    constexpr TileFlag flags() const {
        return static_cast<TileFlag>(value_ & FLAG_MASK);
    }

    /** Get raw value for backward compatibility. */
    constexpr uint16_t raw() const { return value_; }

    /** Check if a flag is set. */
    constexpr bool has(TileFlag flag) const {
        return hasFlag(flags(), flag);
    }

    /** Check if tile has power. */
    constexpr bool isPowered() const { return has(TileFlag::Powered); }

    /** Check if tile can conduct electricity. */
    constexpr bool isConductive() const { return has(TileFlag::Conductive); }

    /** Check if tile can burn. */
    constexpr bool isBurnable() const { return has(TileFlag::Burnable); }

    /** Check if tile can be bulldozed. */
    constexpr bool isBulldozable() const { return has(TileFlag::Bulldozable); }

    /** Check if tile is animated. */
    constexpr bool isAnimated() const { return has(TileFlag::Animated); }

    /** Check if tile is a zone center. */
    constexpr bool isZone() const { return has(TileFlag::Zone); }

    /** Set a flag. */
    TileValue& set(TileFlag flag) {
        value_ |= static_cast<uint16_t>(flag);
        return *this;
    }

    /** Clear a flag. */
    TileValue& clear(TileFlag flag) {
        value_ &= ~static_cast<uint16_t>(flag);
        return *this;
    }

    /** Set the tile number (preserves flags). */
    TileValue& setTile(uint16_t tile) {
        value_ = (value_ & FLAG_MASK) | (tile & TILE_MASK);
        return *this;
    }

    /** Conversion to raw value for backward compatibility. */
    constexpr operator uint16_t() const { return value_; }

private:
    uint16_t value_;
};

/**
 * @brief Common flag combinations.
 */
namespace TileFlags {
    constexpr TileFlag Buildable = TileFlag::Bulldozable | TileFlag::Burnable;
    constexpr TileFlag BuildableConductive =
        TileFlag::Bulldozable | TileFlag::Burnable | TileFlag::Conductive;
    constexpr TileFlag BurnableConductive =
        TileFlag::Burnable | TileFlag::Conductive;
}

#endif // MICROPOLIS_TILE_FLAGS_H
