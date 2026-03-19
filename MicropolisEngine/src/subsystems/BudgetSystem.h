/**
 * @file BudgetSystem.h
 * @brief Budget allocation subsystem.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from budget.cpp to provide a standalone, testable budget
 * calculation system. The BudgetSystem handles:
 * - Budget allocation calculations
 * - Funding percentage management
 * - Budget result computation
 */

#ifndef MICROPOLIS_BUDGET_SYSTEM_H
#define MICROPOLIS_BUDGET_SYSTEM_H

#include "ISubsystem.h"
#include "../events/EventBus.h"
#include "../events/EventTypes.h"
#include "../data_types.h"

namespace MicropolisEngine {

/**
 * @brief Input data for budget calculation.
 */
struct BudgetInput {
    Funds taxFund;        ///< Tax income this period
    Funds totalFunds;     ///< Current treasury balance
    Funds fireFund;       ///< Requested fire department funding
    Funds policeFund;     ///< Requested police department funding
    Funds roadFund;       ///< Requested road maintenance funding
    float firePercent;    ///< Fire budget percentage (0.0-1.0)
    float policePercent;  ///< Police budget percentage (0.0-1.0)
    float roadPercent;    ///< Road budget percentage (0.0-1.0)
};

/**
 * @brief Result of budget calculation.
 */
struct BudgetResult {
    Funds fireValue;      ///< Actual fire department allocation
    Funds policeValue;    ///< Actual police department allocation
    Funds roadValue;      ///< Actual road maintenance allocation
    float firePercent;    ///< Adjusted fire percentage
    float policePercent;  ///< Adjusted police percentage
    float roadPercent;    ///< Adjusted road percentage
    Funds fireSpend;      ///< Fire spending for effect calculation
    Funds policeSpend;    ///< Police spending for effect calculation
    Funds roadSpend;      ///< Road spending for effect calculation
    Funds totalSpend;     ///< Total budget expenditure
    Funds cashChange;     ///< Net change in treasury
    bool needsBudgetWindow; ///< True if budget dialog should show
    bool insufficientFunds; ///< True if couldn't fully fund
};

/**
 * @brief Budget allocation subsystem.
 *
 * Calculates budget allocations based on available funds and
 * requested funding levels. Prioritizes funding in order:
 * roads, fire, police.
 *
 * Usage:
 * @code
 *   BudgetSystem budget(eventBus);
 *   budget.initialize();
 *
 *   BudgetInput input = {
 *       .taxFund = 5000,
 *       .totalFunds = 10000,
 *       .fireFund = 2000,
 *       .policeFund = 1500,
 *       .roadFund = 3000,
 *       .firePercent = 1.0f,
 *       .policePercent = 1.0f,
 *       .roadPercent = 1.0f
 *   };
 *
 *   BudgetResult result = budget.calculate(input, autoBudget, fromMenu);
 * @endcode
 */
class BudgetSystem : public ISubsystem {
public:
    /**
     * @brief Construct a BudgetSystem with injected dependencies.
     * @param eventBus Event bus for publishing budget events.
     */
    explicit BudgetSystem(EventBus& eventBus);

    ~BudgetSystem() override = default;

    // ISubsystem interface
    const char* getName() const override { return "BudgetSystem"; }
    void initialize() override;
    void reset() override;
    bool isInitialized() const override { return initialized_; }

    /**
     * @brief Calculate budget allocation.
     *
     * Allocates funds to fire, police, and roads based on available
     * funds and requested percentages. If insufficient funds, reduces
     * allocations in reverse priority (police first, then fire, then roads).
     *
     * @param input Budget input parameters.
     * @param autoBudget True if auto-budget is enabled.
     * @param fromMenu True if triggered from user menu (not automatic).
     * @return Budget calculation result.
     */
    BudgetResult calculate(const BudgetInput& input, bool autoBudget, bool fromMenu);

    /**
     * @brief Get default initial funding percentages.
     *
     * Returns the default percentages used at game start.
     *
     * @param firePercent Output: fire percentage (1.0)
     * @param policePercent Output: police percentage (1.0)
     * @param roadPercent Output: road percentage (1.0)
     */
    static void getDefaultPercentages(float& firePercent, float& policePercent, float& roadPercent) {
        firePercent = 1.0f;
        policePercent = 1.0f;
        roadPercent = 1.0f;
    }

private:
    EventBus& eventBus_;
    bool initialized_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_BUDGET_SYSTEM_H
