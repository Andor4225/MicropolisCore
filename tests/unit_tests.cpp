/**
 * @file unit_tests.cpp
 * @brief Main entry point for unit tests.
 *
 * This file includes common test utilities and fixtures.
 * Individual test files are compiled separately and linked together.
 */

#include <gtest/gtest.h>
#include "micropolis.h"
#include "callback.h"
#include "world_dimensions.h"

/**
 * @brief Test fixture for Micropolis engine tests.
 *
 * Provides a fresh Micropolis instance for each test.
 */
class MicropolisTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset world dimensions before each test
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();

        // Create engine and set up callback (required to avoid null pointer crash)
        engine = std::make_unique<Micropolis>();
        callback = new ConsoleCallback();
        engine->setCallback(callback, emscripten::val::null());
        engine->init();
    }

    void TearDown() override {
        engine.reset();
        // Note: callback is owned by the engine after setCallback

        // Unlock dimensions for next test
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();
    }

    std::unique_ptr<Micropolis> engine;
    ConsoleCallback* callback = nullptr;
};

/**
 * @brief Test fixture for custom world size tests.
 */
class CustomWorldTest : public ::testing::Test {
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

// Basic sanity test
TEST_F(MicropolisTest, EngineInitializes) {
    EXPECT_NE(engine, nullptr);
    EXPECT_EQ(worldWidth(), 120);
    EXPECT_EQ(worldHeight(), 100);
}

TEST_F(MicropolisTest, DefaultFundsAreSet) {
    EXPECT_GT(engine->totalFunds, 0);
}

TEST_F(MicropolisTest, SimulationRuns) {
    // Run a few simulation ticks
    for (int i = 0; i < 10; i++) {
        engine->simTick();
    }
    // Should not crash
    SUCCEED();
}
