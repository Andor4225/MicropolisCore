/**
 * @file async_scanner.h
 * @brief Asynchronous map scanning utilities.
 *
 * MODERNIZATION (Phase 6.3):
 * Provides parallel execution for independent map scanning operations.
 * Power, traffic, and pollution scans can run concurrently on large maps.
 */

#ifndef MICROPOLIS_ASYNC_SCANNER_H
#define MICROPOLIS_ASYNC_SCANNER_H

#include <future>
#include <functional>
#include <vector>
#include <atomic>

/**
 * @brief Configuration for async scanning.
 */
struct AsyncScanConfig {
    bool enabled = false;           ///< Enable async scanning
    int minWorldSize = 20000;       ///< Minimum tiles to use async (default: 200x100)
    int threadCount = 0;            ///< Thread count (0 = auto-detect)
};

/**
 * @brief Result of an async scan operation.
 */
template<typename T>
struct ScanResult {
    T value;
    bool completed = false;
    std::string error;
};

/**
 * @brief Manager for parallel scan operations.
 *
 * Coordinates async execution of independent scan operations like
 * power grid analysis, traffic density calculation, and pollution spread.
 */
class AsyncScanner {
public:
    AsyncScanner();
    ~AsyncScanner();

    /**
     * @brief Configure async scanning.
     */
    void configure(const AsyncScanConfig& config);

    /**
     * @brief Check if async scanning is enabled and beneficial.
     * @param worldSize Total tiles in the world.
     * @return True if async scanning should be used.
     */
    bool shouldUseAsync(int worldSize) const;

    /**
     * @brief Run a scan operation asynchronously.
     * @param scanFunc The scan function to execute.
     * @return Future for the scan result.
     */
    template<typename Func>
    auto runAsync(Func&& scanFunc) -> std::future<decltype(scanFunc())> {
        return std::async(std::launch::async, std::forward<Func>(scanFunc));
    }

    /**
     * @brief Run multiple scans in parallel and wait for all.
     * @param scans Vector of scan functions.
     */
    template<typename Func>
    void runParallel(std::vector<Func>& scans) {
        std::vector<std::future<void>> futures;
        futures.reserve(scans.size());

        for (auto& scan : scans) {
            futures.push_back(std::async(std::launch::async, scan));
        }

        // Wait for all to complete
        for (auto& f : futures) {
            f.wait();
        }
    }

    /**
     * @brief Get recommended thread count.
     */
    int getThreadCount() const { return threadCount_; }

    /**
     * @brief Check if currently running async operations.
     */
    bool isRunning() const { return activeScans_ > 0; }

private:
    AsyncScanConfig config_;
    int threadCount_;
    std::atomic<int> activeScans_{0};
};

/**
 * @brief RAII guard for tracking active scans.
 */
class ScanGuard {
public:
    ScanGuard(std::atomic<int>& counter) : counter_(counter) {
        counter_++;
    }
    ~ScanGuard() {
        counter_--;
    }

    ScanGuard(const ScanGuard&) = delete;
    ScanGuard& operator=(const ScanGuard&) = delete;

private:
    std::atomic<int>& counter_;
};

/**
 * @brief Parallel map region scanner.
 *
 * Divides the map into regions and processes them in parallel.
 */
class ParallelRegionScanner {
public:
    /**
     * @brief Scan map regions in parallel.
     * @param worldWidth World width in tiles.
     * @param worldHeight World height in tiles.
     * @param regionFunc Function to process each region (startX, startY, endX, endY).
     * @param numThreads Number of parallel threads.
     */
    static void scanRegions(
        int worldWidth,
        int worldHeight,
        std::function<void(int, int, int, int)> regionFunc,
        int numThreads = 0);

    /**
     * @brief Scan with per-tile callback in parallel.
     * @param worldWidth World width.
     * @param worldHeight World height.
     * @param tileFunc Function called for each tile (x, y).
     * @param numThreads Number of parallel threads.
     */
    static void scanTiles(
        int worldWidth,
        int worldHeight,
        std::function<void(int, int)> tileFunc,
        int numThreads = 0);
};

#endif // MICROPOLIS_ASYNC_SCANNER_H
