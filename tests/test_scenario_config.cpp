/**
 * @file test_scenario_config.cpp
 * @brief Unit tests for ScenarioConfig system.
 */

#include <gtest/gtest.h>
#include "config/ScenarioConfig.h"
#include <fstream>
#include <cstdio>

using namespace MicropolisEngine;

class ScenarioConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        testFilePath_ = "test_scenario.json";
    }

    void TearDown() override {
        // Clean up test file
        std::remove(testFilePath_.c_str());
    }

    std::string testFilePath_;
};

TEST_F(ScenarioConfigTest, LoadFromValidJson) {
    std::string json = R"({
        "id": "test_scenario",
        "name": "Test Scenario",
        "description": "A test scenario for unit testing",
        "author": "Test Author",
        "version": "1.0",
        "difficulty": 3,
        "starting": {
            "funds": 50000,
            "taxRate": 8,
            "worldWidth": 150,
            "worldHeight": 120,
            "seed": 12345,
            "generateCity": true
        },
        "rules": {
            "disastersEnabled": false,
            "pollutionMultiplier": 0.5,
            "growthMultiplier": 1.5
        },
        "victories": [
            {
                "type": "population",
                "target": 100000,
                "timeLimit": 0,
                "description": "Reach 100,000 population"
            }
        ],
        "events": [
            {
                "time": 1000,
                "disaster": "fire",
                "x": 50,
                "y": 50,
                "message": "A fire breaks out!"
            }
        ]
    })";

    ScenarioConfig config;
    ScenarioLoadResult result = ScenarioLoader::loadFromString(json, config);

    ASSERT_EQ(result, ScenarioLoadResult::Success);
    EXPECT_EQ(config.id, "test_scenario");
    EXPECT_EQ(config.name, "Test Scenario");
    EXPECT_EQ(config.difficulty, 3);
    EXPECT_EQ(config.starting.funds, 50000);
    EXPECT_EQ(config.starting.taxRate, 8);
    EXPECT_EQ(config.starting.worldWidth, 150);
    EXPECT_EQ(config.starting.seed, 12345);
    EXPECT_TRUE(config.starting.generateCity);
    EXPECT_FALSE(config.rules.disastersEnabled);
    EXPECT_FLOAT_EQ(config.rules.pollutionMultiplier, 0.5f);
    ASSERT_EQ(config.victories.size(), 1);
    EXPECT_EQ(config.victories[0].type, VictoryType::Population);
    EXPECT_EQ(config.victories[0].targetValue, 100000);
    ASSERT_EQ(config.events.size(), 1);
    EXPECT_EQ(config.events[0].disaster, ScenarioDisaster::Fire);
}

TEST_F(ScenarioConfigTest, LoadFromFile) {
    // Write test file
    std::string json = R"({
        "id": "file_test",
        "name": "File Test",
        "victories": [{"type": "score", "target": 500}]
    })";

    std::ofstream file(testFilePath_);
    file << json;
    file.close();

    ScenarioConfig config;
    ScenarioLoadResult result = ScenarioLoader::loadFromFile(testFilePath_, config);

    ASSERT_EQ(result, ScenarioLoadResult::Success);
    EXPECT_EQ(config.id, "file_test");
}

TEST_F(ScenarioConfigTest, MissingRequiredFields) {
    // Missing victories
    std::string json1 = R"({
        "id": "incomplete",
        "name": "Incomplete Scenario"
    })";

    ScenarioConfig config;
    ScenarioLoadResult result = ScenarioLoader::loadFromString(json1, config);
    EXPECT_EQ(result, ScenarioLoadResult::MissingRequiredField);

    // Missing id
    std::string json2 = R"({
        "name": "No ID",
        "victories": [{"type": "population", "target": 1000}]
    })";

    result = ScenarioLoader::loadFromString(json2, config);
    EXPECT_EQ(result, ScenarioLoadResult::MissingRequiredField);
}

TEST_F(ScenarioConfigTest, FileNotFound) {
    ScenarioConfig config;
    ScenarioLoadResult result = ScenarioLoader::loadFromFile("nonexistent.json", config);
    EXPECT_EQ(result, ScenarioLoadResult::FileNotFound);
}

TEST_F(ScenarioConfigTest, SaveAndLoad) {
    ScenarioConfig original;
    original.id = "roundtrip_test";
    original.name = "Roundtrip Test";
    original.description = "Testing save and load";
    original.author = "Unit Test";
    original.difficulty = 2;
    original.starting.funds = 75000;
    original.starting.worldWidth = 200;

    VictoryCondition vc;
    vc.type = VictoryType::Funds;
    vc.targetValue = 1000000;
    vc.description = "Become rich!";
    original.victories.push_back(vc);

    // Save
    ASSERT_TRUE(ScenarioLoader::saveToFile(testFilePath_, original));

    // Load
    ScenarioConfig loaded;
    ScenarioLoadResult result = ScenarioLoader::loadFromFile(testFilePath_, loaded);

    ASSERT_EQ(result, ScenarioLoadResult::Success);
    EXPECT_EQ(loaded.id, original.id);
    EXPECT_EQ(loaded.name, original.name);
    EXPECT_EQ(loaded.starting.funds, original.starting.funds);
    EXPECT_EQ(loaded.victories[0].type, VictoryType::Funds);
    EXPECT_EQ(loaded.victories[0].targetValue, 1000000);
}

TEST_F(ScenarioConfigTest, VictoryTypesParsing) {
    std::string json = R"({
        "id": "victory_types",
        "name": "Victory Types Test",
        "victories": [
            {"type": "population", "target": 10000},
            {"type": "score", "target": 500},
            {"type": "funds", "target": 100000},
            {"type": "survive", "target": 5000}
        ]
    })";

    ScenarioConfig config;
    ScenarioLoadResult result = ScenarioLoader::loadFromString(json, config);

    ASSERT_EQ(result, ScenarioLoadResult::Success);
    ASSERT_EQ(config.victories.size(), 4);
    EXPECT_EQ(config.victories[0].type, VictoryType::Population);
    EXPECT_EQ(config.victories[1].type, VictoryType::Score);
    EXPECT_EQ(config.victories[2].type, VictoryType::Funds);
    EXPECT_EQ(config.victories[3].type, VictoryType::SurviveTime);
}

TEST_F(ScenarioConfigTest, DisasterTypesParsing) {
    std::string json = R"({
        "id": "disasters",
        "name": "Disaster Types Test",
        "victories": [{"type": "survive", "target": 10000}],
        "events": [
            {"time": 100, "disaster": "fire"},
            {"time": 200, "disaster": "flood"},
            {"time": 300, "disaster": "tornado"},
            {"time": 400, "disaster": "earthquake"},
            {"time": 500, "disaster": "monster"},
            {"time": 600, "disaster": "meltdown"}
        ]
    })";

    ScenarioConfig config;
    ScenarioLoadResult result = ScenarioLoader::loadFromString(json, config);

    ASSERT_EQ(result, ScenarioLoadResult::Success);
    ASSERT_EQ(config.events.size(), 6);
    EXPECT_EQ(config.events[0].disaster, ScenarioDisaster::Fire);
    EXPECT_EQ(config.events[1].disaster, ScenarioDisaster::Flood);
    EXPECT_EQ(config.events[2].disaster, ScenarioDisaster::Tornado);
    EXPECT_EQ(config.events[3].disaster, ScenarioDisaster::Earthquake);
    EXPECT_EQ(config.events[4].disaster, ScenarioDisaster::Monster);
    EXPECT_EQ(config.events[5].disaster, ScenarioDisaster::Meltdown);
}

TEST_F(ScenarioConfigTest, EventsSortedByTime) {
    std::string json = R"({
        "id": "sort_test",
        "name": "Sort Test",
        "victories": [{"type": "survive", "target": 10000}],
        "events": [
            {"time": 300, "disaster": "fire"},
            {"time": 100, "disaster": "flood"},
            {"time": 200, "disaster": "tornado"}
        ]
    })";

    ScenarioConfig config;
    ScenarioLoadResult result = ScenarioLoader::loadFromString(json, config);

    ASSERT_EQ(result, ScenarioLoadResult::Success);
    ASSERT_EQ(config.events.size(), 3);
    EXPECT_EQ(config.events[0].triggerTime, 100);
    EXPECT_EQ(config.events[1].triggerTime, 200);
    EXPECT_EQ(config.events[2].triggerTime, 300);
}

TEST_F(ScenarioConfigTest, IsValidCheck) {
    ScenarioConfig valid;
    valid.id = "test";
    valid.name = "Test";
    VictoryCondition vc;
    vc.type = VictoryType::Population;
    valid.victories.push_back(vc);
    EXPECT_TRUE(valid.isValid());

    ScenarioConfig invalid1;
    EXPECT_FALSE(invalid1.isValid()); // Empty id/name/victories

    ScenarioConfig invalid2;
    invalid2.id = "test";
    invalid2.name = "Test";
    EXPECT_FALSE(invalid2.isValid()); // No victories
}

TEST_F(ScenarioConfigTest, GetErrorMessage) {
    EXPECT_EQ(ScenarioLoader::getErrorMessage(ScenarioLoadResult::Success), "Success");
    EXPECT_EQ(ScenarioLoader::getErrorMessage(ScenarioLoadResult::FileNotFound), "Scenario file not found");
    EXPECT_EQ(ScenarioLoader::getErrorMessage(ScenarioLoadResult::MissingRequiredField),
              "Missing required field (id, name, or victories)");
}

// ScenarioRunner tests

class ScenarioRunnerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.id = "runner_test";
        config_.name = "Runner Test";

        VictoryCondition vc;
        vc.type = VictoryType::Population;
        vc.targetValue = 10000;
        config_.victories.push_back(vc);

        ScenarioEvent evt;
        evt.triggerTime = 100;
        evt.disaster = ScenarioDisaster::Fire;
        config_.events.push_back(evt);
    }

    ScenarioConfig config_;
    ScenarioRunner runner_;
};

TEST_F(ScenarioRunnerTest, Initialize) {
    runner_.initialize(config_);
    EXPECT_TRUE(runner_.isActive());
    EXPECT_FALSE(runner_.hasWon());
    EXPECT_FALSE(runner_.hasLost());
    EXPECT_FLOAT_EQ(runner_.getProgress(), 0.0f);
}

TEST_F(ScenarioRunnerTest, TrackProgress) {
    runner_.initialize(config_);

    // 50% progress
    runner_.update(50, 5000, 0, 0);
    EXPECT_FLOAT_EQ(runner_.getProgress(), 0.5f);
    EXPECT_FALSE(runner_.hasWon());

    // 100% progress - win
    runner_.update(100, 10000, 0, 0);
    EXPECT_FLOAT_EQ(runner_.getProgress(), 1.0f);
    EXPECT_TRUE(runner_.hasWon());
}

TEST_F(ScenarioRunnerTest, TriggerEvents) {
    runner_.initialize(config_);

    // Before event time
    runner_.update(50, 0, 0, 0);
    auto events = runner_.getPendingEvents();
    EXPECT_TRUE(events.empty());

    // At event time
    runner_.update(100, 0, 0, 0);
    events = runner_.getPendingEvents();
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].disaster, ScenarioDisaster::Fire);

    // Events should be cleared after getting
    events = runner_.getPendingEvents();
    EXPECT_TRUE(events.empty());
}

TEST_F(ScenarioRunnerTest, TimeLimit) {
    config_.victories[0].timeLimit = 500;

    runner_.initialize(config_);

    // Before time limit
    runner_.update(400, 5000, 0, 0);
    EXPECT_FALSE(runner_.hasLost());

    // At time limit without winning
    runner_.update(500, 5000, 0, 0);
    EXPECT_TRUE(runner_.hasLost());
}

TEST_F(ScenarioRunnerTest, Reset) {
    runner_.initialize(config_);
    runner_.update(100, 10000, 0, 0); // Win

    EXPECT_TRUE(runner_.hasWon());

    runner_.reset();

    EXPECT_FALSE(runner_.isActive());
    EXPECT_FALSE(runner_.hasWon());
    EXPECT_FALSE(runner_.hasLost());
}
