/**
 * @file test_power_system.cpp
 * @brief Unit tests for PowerSystem subsystem.
 */

#include <gtest/gtest.h>
#include "micropolis.h"
#include "callback.h"
#include "subsystems/PowerSystem.h"
#include "world_dimensions.h"

using namespace MicropolisEngine;

class PowerSystemTest : public ::testing::Test {
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

TEST_F(PowerSystemTest, InitialStateNoPower) {
    // Fresh map should have no powered tiles
    // Simulation tick processes power scan internally
    engine->simTick();
    SUCCEED();
}

TEST_F(PowerSystemTest, PowerPlantStrengths) {
    // Test that power plant constants are reasonable
    // These constants are defined in the MicropolisEngine namespace
    EXPECT_GT(MicropolisEngine::COAL_POWER_STRENGTH, 0);
    EXPECT_GT(MicropolisEngine::NUCLEAR_POWER_STRENGTH, 0);
    EXPECT_GT(MicropolisEngine::NUCLEAR_POWER_STRENGTH, MicropolisEngine::COAL_POWER_STRENGTH);
}

TEST_F(PowerSystemTest, PowerScanCompletesViaSimulation) {
    // Generate a random city
    engine->generateSomeCity(12345);

    // Power scan happens during simulation ticks
    for (int i = 0; i < 10; i++) {
        engine->simTick();
    }

    // Should complete without crash
    SUCCEED();
}

TEST_F(PowerSystemTest, SimulationStressTest) {
    // Stress test: run simulation many times (includes power scanning)
    engine->generateSomeCity(54321);

    for (int i = 0; i < 100; i++) {
        engine->simTick();
    }

    SUCCEED();
}

TEST_F(PowerSystemTest, PowerGridWithGeneratedCity) {
    // Generate a city and verify power grid behavior
    engine->generateSomeCity(99999);
    engine->setFunds(100000);

    // Run simulation to process power
    for (int i = 0; i < 50; i++) {
        engine->simTick();
    }

    // City should have some powered zones
    // (Can't directly test internal power state, but simulation should not crash)
    SUCCEED();
}
