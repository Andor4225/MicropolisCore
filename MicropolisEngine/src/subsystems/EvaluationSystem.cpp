/**
 * @file EvaluationSystem.cpp
 * @brief City evaluation and scoring subsystem implementation.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from evaluate.cpp. This implementation maintains the original
 * evaluation logic while providing a cleaner, testable interface.
 */

#include "EvaluationSystem.h"
#include <algorithm>
#include <cmath>

namespace MicropolisEngine {

// Constants for score calculation
constexpr Funds MAX_ROAD_EFFECT = 32;
constexpr Funds MAX_POLICE_STATION_EFFECT = 1000;
constexpr Funds MAX_FIRE_STATION_EFFECT = 1000;

EvaluationSystem::EvaluationSystem(EventBus& eventBus, RngFunction rng)
    : eventBus_(eventBus)
    , rng_(std::move(rng))
    , initialized_(false)
{
}

void EvaluationSystem::initialize()
{
    initialized_ = true;
}

void EvaluationSystem::reset()
{
    // No state to reset
}

EvaluationResult EvaluationSystem::evaluate(const EvaluationInput& input,
                                             Population previousPop, short previousScore)
{
    EvaluationResult result = {};

    if (input.totalPop == 0) {
        // Empty city - return default values
        result = getInitialResult();
        result.cityYes = 50;  // No population => 50/50 approval

        eventBus_.publish(Events::EvaluationComplete{
            result.cityPop,
            result.cityPopDelta,
            result.cityScore,
            result.cityScoreDelta,
            result.cityYes,
            static_cast<int>(result.cityClass)
        });

        return result;
    }

    // Calculate city value
    result.cityAssessedValue = computeAssessedValue(input);

    // Calculate population and class
    result.cityPop = computePopulation(input.resPop, input.comPop, input.indPop);
    if (previousPop == -1) {
        previousPop = result.cityPop;
    }
    result.cityPopDelta = result.cityPop - previousPop;
    result.cityClass = classifyCity(result.cityPop);

    // Calculate problems
    std::array<short, static_cast<int>(CityProblem::NumProblems)> problemTable = {};
    computeProblems(input, problemTable);
    voteOnProblems(problemTable, result.problemVotes);
    sortProblems(result.problemVotes, result.problemOrder);

    // Calculate score
    result.cityScore = computeScore(input, problemTable, previousScore,
                                     result.cityPop, result.cityPopDelta);
    result.cityScoreDelta = result.cityScore - previousScore;

    // Calculate approval
    result.cityYes = computeApproval(result.cityScore);

    // Emit event
    eventBus_.publish(Events::EvaluationComplete{
        result.cityPop,
        result.cityPopDelta,
        result.cityScore,
        result.cityScoreDelta,
        result.cityYes,
        static_cast<int>(result.cityClass)
    });

    return result;
}

Population EvaluationSystem::computePopulation(int resPop, int comPop, int indPop)
{
    return (resPop + (comPop + indPop) * 8LL) * 20LL;
}

CityClassification EvaluationSystem::classifyCity(Population population)
{
    if (population > 500000) {
        return CityClassification::Megalopolis;
    }
    if (population > 100000) {
        return CityClassification::Metropolis;
    }
    if (population > 50000) {
        return CityClassification::Capital;
    }
    if (population > 10000) {
        return CityClassification::City;
    }
    if (population > 2000) {
        return CityClassification::Town;
    }
    return CityClassification::Village;
}

EvaluationResult EvaluationSystem::getInitialResult()
{
    EvaluationResult result = {};
    result.cityYes = 0;
    result.cityPop = 0;
    result.cityPopDelta = 0;
    result.cityAssessedValue = 0;
    result.cityClass = CityClassification::Village;
    result.cityScore = 500;
    result.cityScoreDelta = 0;

    for (int i = 0; i < EVAL_PROBNUM; i++) {
        result.problemVotes[i] = 0;
    }
    for (int i = 0; i < EVAL_PROBLEM_COMPLAINTS; i++) {
        result.problemOrder[i] = static_cast<int>(CityProblem::NumProblems);
    }

    return result;
}

Funds EvaluationSystem::computeAssessedValue(const EvaluationInput& input) const
{
    Funds value = 0;

    value += input.roadTotal * 5;
    value += input.railTotal * 10;
    value += input.policeStationPop * 1000;
    value += input.fireStationPop * 1000;
    value += input.hospitalPop * 400;
    value += input.stadiumPop * 3000;
    value += input.seaportPop * 5000;
    value += input.airportPop * 10000;
    value += input.coalPowerPop * 3000;
    value += input.nuclearPowerPop * 6000;

    return value * 1000;
}

void EvaluationSystem::computeProblems(
    const EvaluationInput& input,
    std::array<short, static_cast<int>(CityProblem::NumProblems)>& problemTable) const
{
    problemTable[static_cast<int>(CityProblem::Crime)] = input.crimeAverage;
    problemTable[static_cast<int>(CityProblem::Pollution)] = input.pollutionAverage;
    problemTable[static_cast<int>(CityProblem::Housing)] = input.landValueAverage * 7 / 10;
    problemTable[static_cast<int>(CityProblem::Taxes)] = input.cityTax * 10;
    problemTable[static_cast<int>(CityProblem::Traffic)] = input.trafficAverage;
    problemTable[static_cast<int>(CityProblem::Unemployment)] =
        computeUnemployment(input.resPop, input.comPop, input.indPop);
    problemTable[static_cast<int>(CityProblem::Fire)] = computeFireSeverity(input.firePop);
}

void EvaluationSystem::voteOnProblems(
    const std::array<short, static_cast<int>(CityProblem::NumProblems)>& problemTable,
    std::array<short, EVAL_PROBNUM>& problemVotes)
{
    for (int i = 0; i < EVAL_PROBNUM; i++) {
        problemVotes[i] = 0;
    }

    int problem = 0;
    int voteCount = 0;
    int loopCount = 0;
    const int numProblems = static_cast<int>(CityProblem::NumProblems);

    while (voteCount < 100 && loopCount < 600) {
        if (problem < numProblems && rng_(300) < problemTable[problem]) {
            problemVotes[problem]++;
            voteCount++;
        }
        problem++;
        if (problem >= numProblems) {
            problem = 0;
        }
        loopCount++;
    }
}

void EvaluationSystem::sortProblems(
    const std::array<short, EVAL_PROBNUM>& problemVotes,
    std::array<int, EVAL_PROBLEM_COMPLAINTS>& problemOrder) const
{
    std::array<bool, EVAL_PROBNUM> problemTaken = {};
    const int numProblems = static_cast<int>(CityProblem::NumProblems);

    for (int z = 0; z < EVAL_PROBLEM_COMPLAINTS; z++) {
        int maxVotes = 0;
        int bestProblem = numProblems;

        for (int i = 0; i < numProblems; i++) {
            if (problemVotes[i] > maxVotes && !problemTaken[i]) {
                bestProblem = i;
                maxVotes = problemVotes[i];
            }
        }

        problemOrder[z] = bestProblem;
        if (bestProblem < numProblems) {
            problemTaken[bestProblem] = true;
        }
    }
}

short EvaluationSystem::computeScore(
    const EvaluationInput& input,
    const std::array<short, static_cast<int>(CityProblem::NumProblems)>& problemTable,
    short previousScore, Population cityPop, Population cityPopDelta) const
{
    int x = 0;
    const int numProblems = static_cast<int>(CityProblem::NumProblems);

    // Sum all problem severities
    for (int z = 0; z < numProblems; z++) {
        x += problemTable[z];
    }

    x = x / 3;
    x = std::min(x, 256);

    // Base score
    int z = std::max(0, std::min((256 - x) * 4, 1000));

    // Penalties for capacity constraints
    if (input.resCap) {
        z = static_cast<int>(z * 0.85);
    }
    if (input.comCap) {
        z = static_cast<int>(z * 0.85);
    }
    if (input.indCap) {
        z = static_cast<int>(z * 0.85);
    }

    // Penalty for poor roads
    if (input.roadEffect < MAX_ROAD_EFFECT) {
        z -= static_cast<int>(MAX_ROAD_EFFECT - input.roadEffect);
    }

    // Penalty for poor police coverage
    if (input.policeEffect < MAX_POLICE_STATION_EFFECT) {
        z = static_cast<int>(z * (0.9 + (input.policeEffect /
                                  (10.0001 * MAX_POLICE_STATION_EFFECT))));
    }

    // Penalty for poor fire coverage
    if (input.fireEffect < MAX_FIRE_STATION_EFFECT) {
        z = static_cast<int>(z * (0.9 + (input.fireEffect /
                                  (10.0001 * MAX_FIRE_STATION_EFFECT))));
    }

    // Penalty for negative demand
    if (input.resValve < -1000) {
        z = static_cast<int>(z * 0.85);
    }
    if (input.comValve < -1000) {
        z = static_cast<int>(z * 0.85);
    }
    if (input.indValve < -1000) {
        z = static_cast<int>(z * 0.85);
    }

    // Population growth modifier
    float SM = 1.0f;
    if (cityPop == 0 || cityPopDelta == 0) {
        SM = 1.0f;
    } else if (cityPopDelta == cityPop) {
        SM = 1.0f;  // City just appeared
    } else if (cityPopDelta > 0) {
        SM = static_cast<float>(cityPopDelta) / cityPop + 1.0f;
    } else if (cityPopDelta < 0) {
        SM = 0.95f + static_cast<float>(cityPopDelta) / (cityPop - cityPopDelta);
    }

    z = static_cast<int>(z * SM);
    z = z - computeFireSeverity(input.firePop) - input.cityTax;

    // Penalty for unpowered zones
    float totalZones = static_cast<float>(input.unpoweredZoneCount + input.poweredZoneCount);
    if (totalZones > 0.0f) {
        z = static_cast<int>(z * (static_cast<float>(input.poweredZoneCount) / totalZones));
    }

    z = std::max(0, std::min(z, 1000));

    // Average with previous score
    return static_cast<short>((previousScore + z) / 2);
}

short EvaluationSystem::computeApproval(short cityScore)
{
    short approval = 0;
    for (int z = 0; z < 100; z++) {
        if (rng_(1000) < cityScore) {
            approval++;
        }
    }
    return approval;
}

short EvaluationSystem::computeUnemployment(int resPop, int comPop, int indPop)
{
    int jobs = (comPop + indPop) * 8;
    if (jobs == 0) {
        return 0;
    }

    float ratio = static_cast<float>(resPop) / jobs;
    int unemployment = static_cast<int>((ratio - 1.0f) * 255);
    return static_cast<short>(std::min(unemployment, 255));
}

short EvaluationSystem::computeFireSeverity(short firePop)
{
    return static_cast<short>(std::min(firePop * 5, 255));
}

} // namespace MicropolisEngine
