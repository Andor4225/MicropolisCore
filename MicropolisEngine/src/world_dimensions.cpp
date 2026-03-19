/**
 * @file world_dimensions.cpp
 * @brief Runtime world dimension management implementation.
 *
 * MODERNIZATION (Phase 4):
 * Implements the WorldDimensions singleton for runtime-configurable map sizes.
 */

#include "world_dimensions.h"
#include <algorithm>

WorldDimensions::WorldDimensions()
    : width_(DEFAULT_WIDTH)
    , height_(DEFAULT_HEIGHT)
    , locked_(false)
{
}

WorldDimensions& WorldDimensions::instance()
{
    static WorldDimensions instance;
    return instance;
}

bool WorldDimensions::setDimensions(int width, int height)
{
    if (locked_) {
        return false;
    }

    // Clamp to valid range
    width_ = std::clamp(width, MIN_WIDTH, MAX_WIDTH);
    height_ = std::clamp(height, MIN_HEIGHT, MAX_HEIGHT);

    return true;
}

void WorldDimensions::lock()
{
    locked_ = true;
}

void WorldDimensions::reset()
{
    if (!locked_) {
        width_ = DEFAULT_WIDTH;
        height_ = DEFAULT_HEIGHT;
    }
}
