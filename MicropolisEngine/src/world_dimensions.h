/**
 * @file world_dimensions.h
 * @brief Runtime world dimension management.
 *
 * MODERNIZATION (Phase 4):
 * Provides runtime-configurable world dimensions, replacing the compile-time
 * constants WORLD_W and WORLD_H. This enables support for custom map sizes.
 *
 * Usage:
 * - Default dimensions (120x100) are used unless explicitly set
 * - Dimensions must be set before Micropolis::init() is called
 * - Once set, dimensions cannot be changed during a session
 */

#ifndef MICROPOLIS_WORLD_DIMENSIONS_H
#define MICROPOLIS_WORLD_DIMENSIONS_H

/**
 * @brief Singleton class managing world dimensions.
 *
 * Provides global access to world dimensions that were previously
 * compile-time constants. Thread-safe for reading after initialization.
 */
class WorldDimensions {
public:
    /** Default world width (classic Micropolis). */
    static constexpr int DEFAULT_WIDTH = 120;

    /** Default world height (classic Micropolis). */
    static constexpr int DEFAULT_HEIGHT = 100;

    /** Minimum supported world width. */
    static constexpr int MIN_WIDTH = 32;

    /** Maximum supported world width. */
    static constexpr int MAX_WIDTH = 512;

    /** Minimum supported world height. */
    static constexpr int MIN_HEIGHT = 32;

    /** Maximum supported world height. */
    static constexpr int MAX_HEIGHT = 512;

    /**
     * @brief Get the singleton instance.
     * @return Reference to the global WorldDimensions instance.
     */
    static WorldDimensions& instance();

    /**
     * @brief Set world dimensions.
     *
     * Must be called before any map allocation. Dimensions are clamped
     * to valid range [MIN, MAX].
     *
     * @param width World width in tiles.
     * @param height World height in tiles.
     * @return True if dimensions were set, false if already locked.
     */
    bool setDimensions(int width, int height);

    /**
     * @brief Lock dimensions (called during initialization).
     *
     * After locking, setDimensions() will fail. This prevents
     * dimension changes after maps are allocated.
     */
    void lock();

    /**
     * @brief Reset to default dimensions (for testing).
     *
     * Only works if not locked.
     */
    void reset();

    /**
     * @brief Unlock dimensions (for testing).
     *
     * Allows changing dimensions after they've been locked.
     * Use with caution - only for test teardown.
     */
    void unlock() { locked_ = false; }

    /**
     * @brief Check if dimensions are locked.
     * @return True if dimensions cannot be changed.
     */
    bool isLocked() const { return locked_; }

    // Dimension accessors
    int width() const { return width_; }
    int height() const { return height_; }

    // Derived dimensions (for compatibility with old WORLD_*_2, etc.)
    int width2() const { return width_ / 2; }
    int height2() const { return height_ / 2; }
    int width4() const { return width_ / 4; }
    int height4() const { return height_ / 4; }
    int width8() const { return width_ / 8; }
    int height8() const { return (height_ + 7) / 8; }

    // Total tile count
    int totalTiles() const { return width_ * height_; }

    // Power stack size (derived from total tiles)
    int powerStackSize() const { return totalTiles() / 4; }

    // Bounds checking
    bool isOnMap(int x, int y) const {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
    }

private:
    WorldDimensions();
    ~WorldDimensions() = default;

    // Non-copyable
    WorldDimensions(const WorldDimensions&) = delete;
    WorldDimensions& operator=(const WorldDimensions&) = delete;

    int width_;
    int height_;
    bool locked_;
};

// ============================================================================
// Global accessor functions (for convenience and C-style compatibility)
// ============================================================================

/**
 * @brief Get world width.
 * @return World width in tiles.
 */
inline int worldWidth() {
    return WorldDimensions::instance().width();
}

/**
 * @brief Get world height.
 * @return World height in tiles.
 */
inline int worldHeight() {
    return WorldDimensions::instance().height();
}

/**
 * @brief Check if position is on the map.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @return True if position is valid.
 */
inline bool isOnMap(int x, int y) {
    return WorldDimensions::instance().isOnMap(x, y);
}

// ============================================================================
// Legacy compatibility macros
// ============================================================================

// These provide drop-in compatibility for code that uses the old constants.
// The compiler will optimize these to direct function calls.

#define WORLD_W_DYNAMIC     (::worldWidth())
#define WORLD_H_DYNAMIC     (::worldHeight())
#define WORLD_W_2_DYNAMIC   (::WorldDimensions::instance().width2())
#define WORLD_H_2_DYNAMIC   (::WorldDimensions::instance().height2())
#define WORLD_W_4_DYNAMIC   (::WorldDimensions::instance().width4())
#define WORLD_H_4_DYNAMIC   (::WorldDimensions::instance().height4())
#define WORLD_W_8_DYNAMIC   (::WorldDimensions::instance().width8())
#define WORLD_H_8_DYNAMIC   (::WorldDimensions::instance().height8())

#endif // MICROPOLIS_WORLD_DIMENSIONS_H
