/**
 * @file test_save_format.cpp
 * @brief Unit tests for modern save format.
 */

#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "micropolis.h"
#include "callback.h"
#include "save_format.h"
#include "world_dimensions.h"

class SaveFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();

        engine = std::make_unique<Micropolis>();
        callback = new ConsoleCallback();
        engine->setCallback(callback, emscripten::val::null());
        engine->init();

        // Generate unique test filename
        testFile = "test_save_" + std::to_string(rand()) + ".mpls";
        testJsonFile = "test_export_" + std::to_string(rand()) + ".json";
    }

    void TearDown() override {
        engine.reset();
        WorldDimensions::instance().unlock();
        WorldDimensions::instance().reset();

        // Clean up test files
        std::remove(testFile.c_str());
        std::remove(testJsonFile.c_str());
    }

    std::unique_ptr<Micropolis> engine;
    ConsoleCallback* callback = nullptr;
    std::string testFile;
    std::string testJsonFile;
};

TEST_F(SaveFormatTest, HeaderSizeIs128Bytes) {
    EXPECT_EQ(sizeof(SaveFileHeader), 128);
}

TEST_F(SaveFormatTest, MagicNumberCorrect) {
    EXPECT_EQ(SAVE_MAGIC, 0x4D504C53);  // "MPLS" in hex
}

TEST_F(SaveFormatTest, SaveResultToStringWorks) {
    EXPECT_STREQ(saveResultToString(SaveResult::SUCCESS), "Success");
    EXPECT_STREQ(saveResultToString(SaveResult::FILE_NOT_FOUND), "File not found");
    EXPECT_STREQ(saveResultToString(SaveResult::INVALID_MAGIC), "Not a valid save file");
}

TEST_F(SaveFormatTest, SaveAndLoadRoundTrip) {
    // Set up some game state
    engine->generateSomeCity(55555);
    engine->setFunds(75000);
    engine->setCityTax(8);
    engine->setCityName("TestCity");

    // Run some simulation
    for (int i = 0; i < 100; i++) {
        engine->simTick();
    }

    Funds savedFunds = engine->totalFunds;
    Quad savedTime = engine->cityTime;

    // Save
    SaveResult saveResult = engine->saveFileModern(testFile, false);
    ASSERT_EQ(saveResult, SaveResult::SUCCESS);

    // Create new engine and load
    auto engine2 = std::make_unique<Micropolis>();
    ConsoleCallback* callback2 = new ConsoleCallback();
    engine2->setCallback(callback2, emscripten::val::null());
    engine2->init();

    SaveResult loadResult = engine2->loadFileModern(testFile);
    ASSERT_EQ(loadResult, SaveResult::SUCCESS);

    // Verify state was preserved
    EXPECT_EQ(engine2->totalFunds, savedFunds);
    EXPECT_EQ(engine2->cityTime, savedTime);
}

TEST_F(SaveFormatTest, SaveWithJsonMetadata) {
    engine->generateSomeCity(66666);

    SaveResult result = engine->saveFileModern(testFile, true);
    EXPECT_EQ(result, SaveResult::SUCCESS);

    // File should exist and be larger than without JSON
    std::ifstream file(testFile, std::ios::binary | std::ios::ate);
    EXPECT_TRUE(file.is_open());
    EXPECT_GT(file.tellg(), sizeof(SaveFileHeader));
}

TEST_F(SaveFormatTest, ExportToJson) {
    engine->generateSomeCity(77777);
    engine->setCityName("JsonTestCity");

    bool result = engine->exportToJson(testJsonFile);
    EXPECT_TRUE(result);

    // Read and verify JSON contains expected content
    std::ifstream file(testJsonFile);
    EXPECT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("JsonTestCity"), std::string::npos);
    EXPECT_NE(content.find("\"width\""), std::string::npos);
    EXPECT_NE(content.find("\"height\""), std::string::npos);
}

TEST_F(SaveFormatTest, LoadNonexistentFile) {
    SaveResult result = engine->loadFileModern("nonexistent_file_12345.mpls");
    EXPECT_EQ(result, SaveResult::FILE_NOT_FOUND);
}

TEST_F(SaveFormatTest, DetectFormatOnModernFile) {
    engine->generateSomeCity(88888);
    engine->saveFileModern(testFile, false);

    bool isModern = false, isLegacy = false;
    SaveResult result = engine->detectSaveFormat(testFile, isModern, isLegacy);

    EXPECT_EQ(result, SaveResult::SUCCESS);
    EXPECT_TRUE(isModern);
    EXPECT_FALSE(isLegacy);
}
