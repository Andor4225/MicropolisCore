/**
 * @file test_world_dimensions.cpp
 * @brief Unit tests for WorldDimensions singleton.
 */

#include <gtest/gtest.h>
#include "world_dimensions.h"

class WorldDimensionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();
    }

    void TearDown() override {
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();
    }
};

TEST_F(WorldDimensionsTest, DefaultDimensions) {
    EXPECT_EQ(worldWidth(), 120);
    EXPECT_EQ(worldHeight(), 100);
}

TEST_F(WorldDimensionsTest, SetCustomDimensions) {
    EXPECT_TRUE(WorldDimensions::instance().setDimensions(200, 150));
    EXPECT_EQ(worldWidth(), 200);
    EXPECT_EQ(worldHeight(), 150);
}

TEST_F(WorldDimensionsTest, ClampsTooSmall) {
    WorldDimensions::instance().setDimensions(10, 10);
    EXPECT_GE(worldWidth(), WorldDimensions::MIN_WIDTH);
    EXPECT_GE(worldHeight(), WorldDimensions::MIN_HEIGHT);
}

TEST_F(WorldDimensionsTest, ClampsTooLarge) {
    WorldDimensions::instance().setDimensions(1000, 1000);
    EXPECT_LE(worldWidth(), WorldDimensions::MAX_WIDTH);
    EXPECT_LE(worldHeight(), WorldDimensions::MAX_HEIGHT);
}

TEST_F(WorldDimensionsTest, LockPreventsChanges) {
    WorldDimensions::instance().setDimensions(200, 150);
    WorldDimensions::instance().lock();

    EXPECT_TRUE(WorldDimensions::instance().isLocked());
    EXPECT_FALSE(WorldDimensions::instance().setDimensions(100, 100));

    // Dimensions should remain unchanged
    EXPECT_EQ(worldWidth(), 200);
    EXPECT_EQ(worldHeight(), 150);
}

TEST_F(WorldDimensionsTest, DerivedDimensions) {
    WorldDimensions::instance().setDimensions(120, 100);

    auto& dims = WorldDimensions::instance();
    EXPECT_EQ(dims.width2(), 60);
    EXPECT_EQ(dims.height2(), 50);
    EXPECT_EQ(dims.width4(), 30);
    EXPECT_EQ(dims.height4(), 25);
    EXPECT_EQ(dims.width8(), 15);
}

TEST_F(WorldDimensionsTest, BoundsChecking) {
    WorldDimensions::instance().setDimensions(120, 100);

    EXPECT_TRUE(isOnMap(0, 0));
    EXPECT_TRUE(isOnMap(119, 99));
    EXPECT_TRUE(isOnMap(60, 50));

    EXPECT_FALSE(isOnMap(-1, 0));
    EXPECT_FALSE(isOnMap(0, -1));
    EXPECT_FALSE(isOnMap(120, 0));
    EXPECT_FALSE(isOnMap(0, 100));
    EXPECT_FALSE(isOnMap(120, 100));
}

TEST_F(WorldDimensionsTest, TotalTiles) {
    WorldDimensions::instance().setDimensions(120, 100);
    EXPECT_EQ(WorldDimensions::instance().totalTiles(), 12000);

    WorldDimensions::instance().setDimensions(200, 200);
    EXPECT_EQ(WorldDimensions::instance().totalTiles(), 40000);
}
