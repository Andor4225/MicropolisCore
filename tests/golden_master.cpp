/**
 * @file golden_master.cpp
 * @brief Golden Master regression test for Micropolis simulation
 *
 * This test harness runs 1000 simulation ticks with a fixed seed and
 * compares the resulting state against a known-good "golden master" snapshot.
 * This ensures simulation determinism is preserved across code changes.
 *
 * Usage:
 *   golden_master           Run test against stored golden master
 *   golden_master --generate Generate new golden master data
 */

#include "micropolis.h"
#include "callback.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>

// Test configuration
static const int TEST_TICKS = 1000;
static const int RANDOM_SEED = 12345;
static const char* GOLDEN_MASTER_FILE = "golden_master.dat";

/**
 * Simulation state snapshot for comparison
 */
struct SimulationSnapshot {
    // Population
    int64_t totalPop;
    int64_t resPop;
    int64_t comPop;
    int64_t indPop;

    // Finances
    int64_t totalFunds;
    int64_t taxFund;
    int64_t cityAssessedValue;

    // Time
    int cityTime;
    int cityMonth;
    int cityYear;

    // Infrastructure
    int64_t roadTotal;
    int64_t railTotal;
    int64_t firePop;
    int64_t policePop;

    // Environment
    int64_t pollutionAverage;
    int64_t crimeAverage;
    int64_t landValueAverage;

    // City score
    int cityScore;
    int cityClass;

    // Checksum of map state (simple hash)
    uint64_t mapChecksum;

    bool operator==(const SimulationSnapshot& other) const {
        return totalPop == other.totalPop &&
               resPop == other.resPop &&
               comPop == other.comPop &&
               indPop == other.indPop &&
               totalFunds == other.totalFunds &&
               taxFund == other.taxFund &&
               cityTime == other.cityTime &&
               cityMonth == other.cityMonth &&
               cityYear == other.cityYear &&
               roadTotal == other.roadTotal &&
               railTotal == other.railTotal &&
               firePop == other.firePop &&
               policePop == other.policePop &&
               pollutionAverage == other.pollutionAverage &&
               crimeAverage == other.crimeAverage &&
               landValueAverage == other.landValueAverage &&
               cityScore == other.cityScore &&
               mapChecksum == other.mapChecksum;
    }

    void print(std::ostream& os) const {
        os << "Simulation Snapshot:\n"
           << "  Population: " << totalPop << " (R:" << resPop
           << " C:" << comPop << " I:" << indPop << ")\n"
           << "  Funds: $" << totalFunds << " (Tax: $" << taxFund << ")\n"
           << "  Time: Year " << cityYear << ", Month " << cityMonth
           << " (tick " << cityTime << ")\n"
           << "  Infrastructure: Roads=" << roadTotal << " Rail=" << railTotal
           << " Fire=" << firePop << " Police=" << policePop << "\n"
           << "  Environment: Pollution=" << pollutionAverage
           << " Crime=" << crimeAverage << " LandValue=" << landValueAverage << "\n"
           << "  City Score: " << cityScore << " Class: " << cityClass << "\n"
           << "  Map Checksum: 0x" << std::hex << mapChecksum << std::dec << "\n";
    }
};

/**
 * Calculate a checksum of the entire map state
 */
uint64_t calculateMapChecksum(Micropolis& sim) {
    uint64_t checksum = 0;
    const uint64_t PRIME = 0x100000001B3ULL;

    for (int x = 0; x < WORLD_W; x++) {
        for (int y = 0; y < WORLD_H; y++) {
            // FNV-1a style hash
            checksum ^= sim.map[x][y];
            checksum *= PRIME;
        }
    }

    return checksum;
}

/**
 * Capture the current simulation state
 */
SimulationSnapshot captureSnapshot(Micropolis& sim) {
    SimulationSnapshot snap;

    snap.totalPop = sim.cityPop;
    snap.resPop = sim.resPop;
    snap.comPop = sim.comPop;
    snap.indPop = sim.indPop;

    snap.totalFunds = sim.totalFunds;
    snap.taxFund = sim.taxFund;
    snap.cityAssessedValue = sim.cityAssessedValue;

    snap.cityTime = sim.cityTime;
    snap.cityMonth = sim.cityMonth;
    snap.cityYear = sim.cityYear;

    snap.roadTotal = sim.roadTotal;
    snap.railTotal = sim.railTotal;
    snap.firePop = sim.firePop;
    snap.policePop = sim.policeStationPop;

    snap.pollutionAverage = sim.pollutionAverage;
    snap.crimeAverage = sim.crimeAverage;
    snap.landValueAverage = sim.landValueAverage;

    snap.cityScore = sim.cityScore;
    snap.cityClass = static_cast<int>(sim.cityClass);

    snap.mapChecksum = calculateMapChecksum(sim);

    return snap;
}

/**
 * Save snapshot to file
 */
bool saveSnapshot(const SimulationSnapshot& snap, const char* filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open " << filename << " for writing\n";
        return false;
    }

    file.write(reinterpret_cast<const char*>(&snap), sizeof(snap));
    return file.good();
}

/**
 * Load snapshot from file
 */
bool loadSnapshot(SimulationSnapshot& snap, const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open " << filename << " for reading\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(&snap), sizeof(snap));
    return file.good();
}

/**
 * Run the simulation for a fixed number of ticks
 */
void runSimulation(Micropolis& sim, int ticks) {
    for (int i = 0; i < ticks; i++) {
        sim.simTick();

        // Progress indicator every 100 ticks
        if ((i + 1) % 100 == 0) {
            std::cout << "  Tick " << (i + 1) << "/" << ticks << "\r" << std::flush;
        }
    }
    std::cout << std::endl;
}

/**
 * Initialize simulation with deterministic state
 */
void initializeTestSimulation(Micropolis& sim) {
    // Initialize with a fixed seed for reproducibility
    sim.seedRandom(RANDOM_SEED);

    // Generate a map (this uses the random seed)
    sim.generateMap(RANDOM_SEED);

    // Set initial game state
    sim.setFunds(20000);
    sim.setSpeed(3);  // Fast speed
    sim.setGameLevel(LEVEL_EASY);  // Easy

    // Disable disasters for deterministic testing
    sim.enableDisasters = false;

    // Enable auto-budget to remove human interaction
    sim.autoBudget = true;
    sim.autoBulldoze = true;
}

/**
 * Print differences between two snapshots
 */
void printDifferences(const SimulationSnapshot& expected,
                      const SimulationSnapshot& actual) {
    std::cout << "\n=== DIFFERENCES ===\n\n";

    #define CHECK_FIELD(field) \
        if (expected.field != actual.field) { \
            std::cout << "  " #field ": expected " << expected.field \
                      << ", got " << actual.field << "\n"; \
        }

    CHECK_FIELD(totalPop);
    CHECK_FIELD(resPop);
    CHECK_FIELD(comPop);
    CHECK_FIELD(indPop);
    CHECK_FIELD(totalFunds);
    CHECK_FIELD(taxFund);
    CHECK_FIELD(cityTime);
    CHECK_FIELD(cityMonth);
    CHECK_FIELD(cityYear);
    CHECK_FIELD(roadTotal);
    CHECK_FIELD(railTotal);
    CHECK_FIELD(firePop);
    CHECK_FIELD(policePop);
    CHECK_FIELD(pollutionAverage);
    CHECK_FIELD(crimeAverage);
    CHECK_FIELD(landValueAverage);
    CHECK_FIELD(cityScore);

    if (expected.mapChecksum != actual.mapChecksum) {
        std::cout << "  mapChecksum: expected 0x" << std::hex << expected.mapChecksum
                  << ", got 0x" << actual.mapChecksum << std::dec << "\n";
    }

    #undef CHECK_FIELD
}

int main(int argc, char* argv[]) {
    bool generateMode = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--generate") == 0 || strcmp(argv[i], "-g") == 0) {
            generateMode = true;
        }
    }

    std::cout << "=== Micropolis Golden Master Test ===\n\n";
    std::cout << "Configuration:\n";
    std::cout << "  Ticks: " << TEST_TICKS << "\n";
    std::cout << "  Seed: " << RANDOM_SEED << "\n";
    std::cout << "  Mode: " << (generateMode ? "GENERATE" : "VERIFY") << "\n\n";

    // Create and initialize simulation
    Micropolis sim;

    // Set up a console callback to avoid null pointer dereference
    ConsoleCallback* callback = new ConsoleCallback();
    sim.setCallback(callback, emscripten::val::null());

    sim.init();
    initializeTestSimulation(sim);

    std::cout << "Running simulation...\n";
    runSimulation(sim, TEST_TICKS);

    // Capture final state
    SimulationSnapshot currentSnapshot = captureSnapshot(sim);

    if (generateMode) {
        // Generate new golden master
        std::cout << "\nGenerating new golden master...\n";
        currentSnapshot.print(std::cout);

        if (saveSnapshot(currentSnapshot, GOLDEN_MASTER_FILE)) {
            std::cout << "\nGolden master saved to " << GOLDEN_MASTER_FILE << "\n";
            return 0;
        } else {
            std::cerr << "\nFailed to save golden master!\n";
            return 1;
        }
    } else {
        // Verify against golden master
        SimulationSnapshot goldenSnapshot;

        if (!loadSnapshot(goldenSnapshot, GOLDEN_MASTER_FILE)) {
            std::cerr << "\nNo golden master file found. Run with --generate first.\n";
            std::cout << "\nCurrent simulation state:\n";
            currentSnapshot.print(std::cout);
            return 2;
        }

        std::cout << "\nComparing against golden master...\n";

        if (currentSnapshot == goldenSnapshot) {
            std::cout << "\n=== TEST PASSED ===\n";
            std::cout << "Simulation is deterministic and matches golden master.\n";
            return 0;
        } else {
            std::cout << "\n=== TEST FAILED ===\n";
            std::cout << "Simulation diverged from golden master!\n";

            std::cout << "\nExpected (golden master):\n";
            goldenSnapshot.print(std::cout);

            std::cout << "\nActual (current):\n";
            currentSnapshot.print(std::cout);

            printDifferences(goldenSnapshot, currentSnapshot);

            return 1;
        }
    }
}
