/**
 * @file async_scanner.cpp
 * @brief Implementation of asynchronous map scanning utilities.
 *
 * MODERNIZATION (Phase 6.3):
 * Provides parallel execution for independent map scanning operations.
 */

#include "async_scanner.h"
#include <thread>
#include <algorithm>

AsyncScanner::AsyncScanner()
    : threadCount_(0)
{
    // Auto-detect thread count based on hardware
    threadCount_ = static_cast<int>(std::thread::hardware_concurrency());
    if (threadCount_ == 0) {
        threadCount_ = 2; // Fallback to 2 threads if detection fails
    }
}

AsyncScanner::~AsyncScanner() = default;

void AsyncScanner::configure(const AsyncScanConfig& config)
{
    config_ = config;

    if (config.threadCount > 0) {
        threadCount_ = config.threadCount;
    } else {
        // Auto-detect
        threadCount_ = static_cast<int>(std::thread::hardware_concurrency());
        if (threadCount_ == 0) {
            threadCount_ = 2;
        }
    }
}

bool AsyncScanner::shouldUseAsync(int worldSize) const
{
    if (!config_.enabled) {
        return false;
    }

    return worldSize >= config_.minWorldSize;
}

// ParallelRegionScanner implementation

void ParallelRegionScanner::scanRegions(
    int worldWidth,
    int worldHeight,
    std::function<void(int, int, int, int)> regionFunc,
    int numThreads)
{
    if (numThreads <= 0) {
        numThreads = static_cast<int>(std::thread::hardware_concurrency());
        if (numThreads == 0) {
            numThreads = 2;
        }
    }

    // Don't use more threads than we have rows
    numThreads = std::min(numThreads, worldHeight);

    if (numThreads <= 1) {
        // Single-threaded fallback
        regionFunc(0, 0, worldWidth, worldHeight);
        return;
    }

    // Divide the map into horizontal strips
    int rowsPerThread = worldHeight / numThreads;
    int remainingRows = worldHeight % numThreads;

    std::vector<std::future<void>> futures;
    futures.reserve(numThreads);

    int startY = 0;
    for (int i = 0; i < numThreads; ++i) {
        int endY = startY + rowsPerThread;
        if (i < remainingRows) {
            endY++; // Distribute remaining rows
        }

        futures.push_back(std::async(std::launch::async,
            [regionFunc, worldWidth, startY, endY]() {
                regionFunc(0, startY, worldWidth, endY);
            }
        ));

        startY = endY;
    }

    // Wait for all threads to complete
    for (auto& f : futures) {
        f.wait();
    }
}

void ParallelRegionScanner::scanTiles(
    int worldWidth,
    int worldHeight,
    std::function<void(int, int)> tileFunc,
    int numThreads)
{
    // Wrap the per-tile function in a region function
    auto regionWrapper = [&tileFunc](int startX, int startY, int endX, int endY) {
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                tileFunc(x, y);
            }
        }
    };

    scanRegions(worldWidth, worldHeight, regionWrapper, numThreads);
}
