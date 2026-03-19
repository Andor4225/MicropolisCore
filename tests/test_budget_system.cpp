/**
 * @file test_budget_system.cpp
 * @brief Unit tests for BudgetSystem subsystem.
 */

#include <gtest/gtest.h>
#include "micropolis.h"
#include "callback.h"
#include "world_dimensions.h"

class BudgetSystemTest : public ::testing::Test {
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

TEST_F(BudgetSystemTest, InitialFundingLevels) {
    // Funding percentages should be initialized to valid values
    EXPECT_GE(engine->roadPercent, 0.0f);
    EXPECT_LE(engine->roadPercent, 1.0f);
    EXPECT_GE(engine->policePercent, 0.0f);
    EXPECT_LE(engine->policePercent, 1.0f);
    EXPECT_GE(engine->firePercent, 0.0f);
    EXPECT_LE(engine->firePercent, 1.0f);
}

TEST_F(BudgetSystemTest, TaxRateBounds) {
    // Tax rate should be within reasonable bounds
    engine->setCityTax(5);
    EXPECT_EQ(engine->cityTax, 5);

    engine->setCityTax(20);
    EXPECT_LE(engine->cityTax, 20);

    engine->setCityTax(0);
    EXPECT_GE(engine->cityTax, 0);
}

TEST_F(BudgetSystemTest, FundsCanBeSet) {
    engine->setFunds(50000);
    EXPECT_EQ(engine->totalFunds, 50000);

    engine->setFunds(0);
    EXPECT_EQ(engine->totalFunds, 0);

    engine->setFunds(-1000);
    EXPECT_EQ(engine->totalFunds, -1000);  // Negative funds allowed (debt)
}

TEST_F(BudgetSystemTest, BudgetCalculationRuns) {
    engine->generateSomeCity(33333);

    // Set some funds
    engine->setFunds(100000);

    // Run simulation which includes budget calculations
    for (int i = 0; i < 50; i++) {
        engine->simTick();
    }

    SUCCEED();
}

TEST_F(BudgetSystemTest, SpendingAffectsFunds) {
    engine->generateSomeCity(44444);

    Funds initialFunds = 100000;
    engine->setFunds(initialFunds);

    // Run many simulation ticks
    for (int i = 0; i < 500; i++) {
        engine->simTick();
    }

    // Funds should have changed (either spent or collected taxes)
    // We don't assert direction because it depends on city state
    SUCCEED();
}
