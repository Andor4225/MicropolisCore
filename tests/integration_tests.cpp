/**
 * @file integration_tests.cpp
 * @brief Integration tests for Micropolis engine.
 *
 * Tests that verify multiple subsystems work together correctly.
 */

#include <gtest/gtest.h>
#include "micropolis.h"
#include "callback.h"
#include "world_dimensions.h"
#include "config/GameConfig.h"

using namespace MicropolisEngine;

/**
 * @brief Helper to create and initialize a Micropolis engine with callback.
 */
std::unique_ptr<Micropolis> createEngine() {
    auto engine = std::make_unique<Micropolis>();
    ConsoleCallback* callback = new ConsoleCallback();
    engine->setCallback(callback, emscripten::val::null());
    engine->init();
    return engine;
}

class IntegrationTest : public ::testing::Test {
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

TEST_F(IntegrationTest, FullSimulationCycle) {
    auto engine = createEngine();
    engine->generateSomeCity(12345);
    engine->setFunds(100000);

    // Run 1000 simulation ticks
    for (int i = 0; i < 1000; i++) {
        engine->simTick();
    }

    // City should have developed
    EXPECT_GT(engine->cityTime, 0);
    SUCCEED();
}

TEST_F(IntegrationTest, CustomWorldSizeSimulation) {
    // Set custom dimensions before creating engine
    WorldDimensions::instance().setDimensions(200, 150);

    auto engine = createEngine();

    EXPECT_EQ(worldWidth(), 200);
    EXPECT_EQ(worldHeight(), 150);

    // Generate and run simulation on larger map
    engine->generateSomeCity(54321);

    for (int i = 0; i < 500; i++) {
        engine->simTick();
    }

    SUCCEED();
}

TEST_F(IntegrationTest, ConfigAppliedToSimulation) {
    GameConfig config;
    config.budget.startingFunds = 50000;
    config.budget.defaultTaxRate = 5;

    auto engine = createEngine();

    // Apply config values
    engine->setFunds(static_cast<int>(config.budget.startingFunds));
    engine->setCityTax(static_cast<short>(config.budget.defaultTaxRate));

    EXPECT_EQ(engine->totalFunds, 50000);
    EXPECT_EQ(engine->cityTax, 5);
}

TEST_F(IntegrationTest, DisasterSystemIntegration) {
    auto engine = createEngine();
    engine->generateSomeCity(99999);

    // Enable disasters and run simulation
    engine->enableDisasters = true;

    for (int i = 0; i < 1000; i++) {
        engine->simTick();
    }

    // Should complete without crash
    SUCCEED();
}

TEST_F(IntegrationTest, SaveLoadPreservesSimulationState) {
    std::string testFile = "integration_test_save.mpls";

    // Create and simulate city
    auto engine1 = createEngine();
    engine1->generateSomeCity(11111);
    engine1->setFunds(75000);

    for (int i = 0; i < 500; i++) {
        engine1->simTick();
    }

    Quad savedTime = engine1->cityTime;

    // Save
    SaveResult result = engine1->saveFileModern(testFile, false);
    ASSERT_EQ(result, SaveResult::SUCCESS);

    // Load into new engine
    auto engine2 = createEngine();
    result = engine2->loadFileModern(testFile);
    ASSERT_EQ(result, SaveResult::SUCCESS);

    // Continue simulation
    for (int i = 0; i < 500; i++) {
        engine2->simTick();
    }

    // Time should have advanced from saved state
    EXPECT_GT(engine2->cityTime, savedTime);

    // Clean up
    std::remove(testFile.c_str());
}

TEST_F(IntegrationTest, MultipleEngineInstances) {
    // Test that multiple engine instances don't interfere
    auto engine1 = createEngine();
    auto engine2 = createEngine();

    engine1->generateSomeCity(11111);
    engine2->generateSomeCity(22222);

    engine1->setFunds(100000);
    engine2->setFunds(200000);

    // They should have different funds
    EXPECT_NE(engine1->totalFunds, engine2->totalFunds);

    // Run both simulations
    for (int i = 0; i < 100; i++) {
        engine1->simTick();
        engine2->simTick();
    }

    // Should complete without interference
    SUCCEED();
}

TEST_F(IntegrationTest, ZeroFundsSimulation) {
    auto engine = createEngine();
    engine->generateSomeCity(33333);
    engine->setFunds(0);

    // Simulation should handle zero funds gracefully
    for (int i = 0; i < 500; i++) {
        engine->simTick();
    }

    SUCCEED();
}

TEST_F(IntegrationTest, HighSpeedSimulation) {
    auto engine = createEngine();
    engine->generateSomeCity(44444);
    engine->setSpeed(3);  // Fastest speed

    // Run at high speed
    for (int i = 0; i < 2000; i++) {
        engine->simTick();
    }

    SUCCEED();
}
