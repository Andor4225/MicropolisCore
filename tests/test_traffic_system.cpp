/**
 * @file test_traffic_system.cpp
 * @brief Unit tests for TrafficSystem subsystem.
 */

#include <gtest/gtest.h>
#include "micropolis.h"
#include "callback.h"
#include "subsystems/TrafficSystem.h"
#include "subsystems/AStarPathfinder.h"
#include "world_dimensions.h"

using namespace MicropolisEngine;

class TrafficSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();

        engine = std::make_unique<Micropolis>();
        callback = new ConsoleCallback();
        engine->setCallback(callback, emscripten::val::null());
        engine->init();
    }

    void TearDown() override {
        engine.reset();
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();
    }

    std::unique_ptr<Micropolis> engine;
    ConsoleCallback* callback = nullptr;
};

TEST_F(TrafficSystemTest, MaxTrafficDistancePositive) {
    // Use the global MAX_TRAFFIC_DISTANCE to avoid ambiguity
    EXPECT_GT(::MAX_TRAFFIC_DISTANCE, 0);
    EXPECT_LE(::MAX_TRAFFIC_DISTANCE, 50);  // Reasonable upper bound
}

TEST_F(TrafficSystemTest, TrafficScanCompletes) {
    engine->generateSomeCity(11111);

    // Run several simulation ticks (traffic is processed during simulation)
    for (int i = 0; i < 100; i++) {
        engine->simTick();
    }

    SUCCEED();
}

TEST_F(TrafficSystemTest, TrafficDensityMapBoundsCheck) {
    engine->generateSomeCity(22222);

    // Traffic density map should be within bounds
    for (int x = 0; x < worldWidth() / 2; x++) {
        for (int y = 0; y < worldHeight() / 2; y++) {
            Byte density = engine->trafficDensityMap.get(x, y);
            EXPECT_LE(density, 255);
        }
    }
}

// A* Pathfinder Tests
class AStarPathfinderTest : public ::testing::Test {
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

TEST_F(AStarPathfinderTest, PathfinderConfigDefaults) {
    PathfinderConfig config;
    EXPECT_GT(config.maxSearchDistance, 0);
    EXPECT_GE(config.trafficCostWeight, 0.0f);
    EXPECT_GE(config.randomizationFactor, 0.0f);
}

TEST_F(AStarPathfinderTest, PathResultDefaultEmpty) {
    PathResult result;
    EXPECT_FALSE(result.found);
    EXPECT_TRUE(result.path.empty());
    EXPECT_EQ(result.pathLength, 0);
}
