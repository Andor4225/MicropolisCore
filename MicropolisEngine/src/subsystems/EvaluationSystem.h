/**
 * @file EvaluationSystem.h
 * @brief City evaluation and scoring subsystem.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from evaluate.cpp to provide a standalone, testable evaluation
 * system. The EvaluationSystem handles:
 * - City population calculation and classification
 * - City value assessment
 * - Problem identification and voting
 * - Score calculation and mayor approval rating
 */

#ifndef MICROPOLIS_EVALUATION_SYSTEM_H
#define MICROPOLIS_EVALUATION_SYSTEM_H

#include "ISubsystem.h"
#include "../events/EventBus.h"
#include "../events/EventTypes.h"
#include "../data_types.h"
#include <functional>
#include <array>

namespace MicropolisEngine {

/** Number of problem types citizens can vote on. */
constexpr int EVAL_PROBNUM = 10;

/** Number of top problems to track. */
constexpr int EVAL_PROBLEM_COMPLAINTS = 4;

/**
 * @brief City class enumeration based on population.
 */
enum class CityClassification {
    Village = 0,
    Town = 1,
    City = 2,
    Capital = 3,
    Metropolis = 4,
    Megalopolis = 5
};

/**
 * @brief City voting problem types.
 */
enum class CityProblem {
    Crime = 0,
    Pollution = 1,
    Housing = 2,
    Taxes = 3,
    Traffic = 4,
    Unemployment = 5,
    Fire = 6,
    NumProblems = 7
};

/**
 * @brief Input data for city evaluation.
 */
struct EvaluationInput {
    // Population data
    int totalPop;
    int resPop;
    int comPop;
    int indPop;

    // Infrastructure counts
    int roadTotal;
    int railTotal;
    int policeStationPop;
    int fireStationPop;
    int hospitalPop;
    int stadiumPop;
    int seaportPop;
    int airportPop;
    int coalPowerPop;
    int nuclearPowerPop;

    // Problem metrics
    short crimeAverage;
    short pollutionAverage;
    short landValueAverage;
    short cityTax;
    short trafficAverage;
    short firePop;

    // Capacity flags
    bool resCap;
    bool comCap;
    bool indCap;

    // Service effects
    Funds roadEffect;
    Funds policeEffect;
    Funds fireEffect;

    // Demand valves
    short resValve;
    short comValve;
    short indValve;

    // Zone power status
    int poweredZoneCount;
    int unpoweredZoneCount;
};

/**
 * @brief Result of city evaluation.
 */
struct EvaluationResult {
    Population cityPop;
    Population cityPopDelta;
    Funds cityAssessedValue;
    CityClassification cityClass;
    short cityScore;
    short cityScoreDelta;
    short cityYes;  ///< Mayor approval (0-100)

    std::array<short, EVAL_PROBNUM> problemVotes;
    std::array<int, EVAL_PROBLEM_COMPLAINTS> problemOrder;
};

/**
 * @brief City evaluation and scoring subsystem.
 *
 * Calculates city population, assessed value, city class, problems,
 * and overall score. Emits events when evaluation completes.
 *
 * Usage:
 * @code
 *   EvaluationSystem eval(eventBus, [this](short n) { return getRandom(n); });
 *   eval.initialize();
 *
 *   EvaluationInput input = { ... };
 *   EvaluationResult result = eval.evaluate(input, previousPop, previousScore);
 * @endcode
 */
class EvaluationSystem : public ISubsystem {
public:
    /** Random number generator function type. */
    using RngFunction = std::function<short(short)>;

    /**
     * @brief Construct an EvaluationSystem with injected dependencies.
     * @param eventBus Event bus for publishing evaluation events.
     * @param rng Random number generator function.
     */
    EvaluationSystem(EventBus& eventBus, RngFunction rng);

    ~EvaluationSystem() override = default;

    // ISubsystem interface
    const char* getName() const override { return "EvaluationSystem"; }
    void initialize() override;
    void reset() override;
    bool isInitialized() const override { return initialized_; }

    /**
     * @brief Perform city evaluation.
     *
     * Calculates all evaluation metrics based on input data.
     *
     * @param input Current city state data.
     * @param previousPop Previous population (for delta calculation).
     * @param previousScore Previous score (for delta calculation).
     * @return Evaluation results.
     */
    EvaluationResult evaluate(const EvaluationInput& input,
                              Population previousPop, short previousScore);

    /**
     * @brief Compute city population from zone populations.
     * @param resPop Residential population.
     * @param comPop Commercial population.
     * @param indPop Industrial population.
     * @return Total city population.
     */
    static Population computePopulation(int resPop, int comPop, int indPop);

    /**
     * @brief Classify city based on population.
     * @param population City population.
     * @return City classification.
     */
    static CityClassification classifyCity(Population population);

    /**
     * @brief Get initial/default evaluation result.
     * @return Default evaluation result for empty city.
     */
    static EvaluationResult getInitialResult();

private:
    /**
     * @brief Calculate city assessed value.
     * @param input City state data.
     * @return Assessed value in dollars.
     */
    Funds computeAssessedValue(const EvaluationInput& input) const;

    /**
     * @brief Compute problem severity values.
     * @param input City state data.
     * @param problemTable Output: severity of each problem.
     */
    void computeProblems(const EvaluationInput& input,
                         std::array<short, static_cast<int>(CityProblem::NumProblems)>& problemTable) const;

    /**
     * @brief Vote on city problems.
     * @param problemTable Problem severities.
     * @param problemVotes Output: votes for each problem.
     */
    void voteOnProblems(const std::array<short, static_cast<int>(CityProblem::NumProblems)>& problemTable,
                        std::array<short, EVAL_PROBNUM>& problemVotes);

    /**
     * @brief Sort problems by vote count.
     * @param problemVotes Votes for each problem.
     * @param problemOrder Output: indices of problems in decreasing severity.
     */
    void sortProblems(const std::array<short, EVAL_PROBNUM>& problemVotes,
                      std::array<int, EVAL_PROBLEM_COMPLAINTS>& problemOrder) const;

    /**
     * @brief Compute overall city score.
     * @param input City state data.
     * @param problemTable Problem severities.
     * @param previousScore Previous city score.
     * @param cityPop Current city population.
     * @param cityPopDelta Population change.
     * @return New city score (0-1000).
     */
    short computeScore(const EvaluationInput& input,
                       const std::array<short, static_cast<int>(CityProblem::NumProblems)>& problemTable,
                       short previousScore, Population cityPop, Population cityPopDelta) const;

    /**
     * @brief Compute mayor approval rating.
     * @param cityScore Current city score.
     * @return Approval rating (0-100).
     */
    short computeApproval(short cityScore);

    /**
     * @brief Compute unemployment severity.
     * @param resPop Residential population.
     * @param comPop Commercial population.
     * @param indPop Industrial population.
     * @return Unemployment severity (0-255).
     */
    static short computeUnemployment(int resPop, int comPop, int indPop);

    /**
     * @brief Compute fire severity.
     * @param firePop Fire count.
     * @return Fire severity (0-255).
     */
    static short computeFireSeverity(short firePop);

    EventBus& eventBus_;
    RngFunction rng_;
    bool initialized_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_EVALUATION_SYSTEM_H
