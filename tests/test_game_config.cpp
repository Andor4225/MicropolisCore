/**
 * @file test_game_config.cpp
 * @brief Unit tests for GameConfig system.
 */

#include <gtest/gtest.h>
#include <cstdio>
#include "config/GameConfig.h"

using namespace MicropolisEngine;

class GameConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        testConfigFile = "test_config_" + std::to_string(rand()) + ".json";
    }

    void TearDown() override {
        std::remove(testConfigFile.c_str());
    }

    std::string testConfigFile;
};

TEST_F(GameConfigTest, DefaultValuesAreReasonable) {
    GameConfig config;

    // Traffic defaults
    EXPECT_GT(config.traffic.maxSearchDistance, 0);
    EXPECT_GE(config.traffic.trafficCostWeight, 0.0f);

    // Power defaults
    EXPECT_GT(config.power.coalPowerStrength, 0);
    EXPECT_GT(config.power.nuclearPowerStrength, 0);

    // Budget defaults
    EXPECT_GE(config.budget.defaultTaxRate, 0);
    EXPECT_LE(config.budget.defaultTaxRate, 20);
    EXPECT_GT(config.budget.startingFunds, 0);

    // World defaults
    EXPECT_EQ(config.world.width, 120);
    EXPECT_EQ(config.world.height, 100);
}

TEST_F(GameConfigTest, LoadFromString) {
    GameConfig config;

    std::string json = R"({
        "traffic": {
            "maxSearchDistance": 50,
            "useAStar": true
        },
        "power": {
            "coalPowerStrength": 800
        }
    })";

    bool result = config.loadFromString(json);
    EXPECT_TRUE(result);

    EXPECT_EQ(config.traffic.maxSearchDistance, 50);
    EXPECT_TRUE(config.traffic.useAStar);
    EXPECT_EQ(config.power.coalPowerStrength, 800);
}

TEST_F(GameConfigTest, SaveAndLoadRoundTrip) {
    GameConfig config1;
    config1.traffic.maxSearchDistance = 40;
    config1.traffic.useAStar = true;
    config1.power.coalPowerStrength = 750;
    config1.budget.defaultTaxRate = 9;
    config1.world.width = 200;
    config1.world.height = 150;

    // Save
    EXPECT_TRUE(config1.saveToFile(testConfigFile));

    // Load into new config
    GameConfig config2;
    EXPECT_TRUE(config2.loadFromFile(testConfigFile));

    // Verify
    EXPECT_EQ(config2.traffic.maxSearchDistance, 40);
    EXPECT_TRUE(config2.traffic.useAStar);
    EXPECT_EQ(config2.power.coalPowerStrength, 750);
    EXPECT_EQ(config2.budget.defaultTaxRate, 9);
    EXPECT_EQ(config2.world.width, 200);
    EXPECT_EQ(config2.world.height, 150);
}

TEST_F(GameConfigTest, ResetToDefaults) {
    GameConfig config;
    config.traffic.maxSearchDistance = 100;
    config.power.coalPowerStrength = 1000;

    config.resetToDefaults();

    EXPECT_EQ(config.traffic.maxSearchDistance, 30);  // Default
    EXPECT_EQ(config.power.coalPowerStrength, 700);   // Default
}

TEST_F(GameConfigTest, ToJsonString) {
    GameConfig config;
    std::string json = config.toJsonString();

    // Should contain all sections
    EXPECT_NE(json.find("\"traffic\""), std::string::npos);
    EXPECT_NE(json.find("\"power\""), std::string::npos);
    EXPECT_NE(json.find("\"budget\""), std::string::npos);
    EXPECT_NE(json.find("\"disasters\""), std::string::npos);
    EXPECT_NE(json.find("\"world\""), std::string::npos);
    EXPECT_NE(json.find("\"simulation\""), std::string::npos);
}

TEST_F(GameConfigTest, LoadNonexistentFile) {
    GameConfig config;
    bool result = config.loadFromFile("nonexistent_config_12345.json");
    EXPECT_FALSE(result);
    EXPECT_FALSE(config.getLastError().empty());
}

TEST_F(GameConfigTest, PartialConfigPreservesDefaults) {
    GameConfig config;

    // Only set traffic section
    std::string json = R"({
        "traffic": {
            "maxSearchDistance": 60
        }
    })";

    config.loadFromString(json);

    // Traffic should be updated
    EXPECT_EQ(config.traffic.maxSearchDistance, 60);

    // Other sections should retain defaults
    EXPECT_EQ(config.power.coalPowerStrength, 700);
    EXPECT_EQ(config.budget.defaultTaxRate, 7);
}
