/**
 * @file BudgetSystem.cpp
 * @brief Budget allocation subsystem implementation.
 *
 * MODERNIZATION (Phase 3):
 * Extracted from budget.cpp. This implementation maintains the original
 * budget allocation algorithm while providing a cleaner, testable interface.
 */

#include "BudgetSystem.h"
#include <cassert>

namespace MicropolisEngine {

BudgetSystem::BudgetSystem(EventBus& eventBus)
    : eventBus_(eventBus)
    , initialized_(false)
{
}

void BudgetSystem::initialize()
{
    initialized_ = true;
}

void BudgetSystem::reset()
{
    // No state to reset - budget system is stateless
}

BudgetResult BudgetSystem::calculate(const BudgetInput& input, bool autoBudget, bool fromMenu)
{
    BudgetResult result = {};
    result.firePercent = input.firePercent;
    result.policePercent = input.policePercent;
    result.roadPercent = input.roadPercent;

    // Calculate requested amounts based on percentages
    Funds fireInt = static_cast<Funds>(input.fireFund * input.firePercent);
    Funds policeInt = static_cast<Funds>(input.policeFund * input.policePercent);
    Funds roadInt = static_cast<Funds>(input.roadFund * input.roadPercent);

    Funds total = fireInt + policeInt + roadInt;
    Funds yumDuckets = input.taxFund + input.totalFunds;

    result.insufficientFunds = false;

    if (yumDuckets > total) {
        // Enough funds to fully fund everything
        result.fireValue = fireInt;
        result.policeValue = policeInt;
        result.roadValue = roadInt;

    } else if (total > 0) {
        // Not enough funds - prioritize roads, fire, police
        result.insufficientFunds = true;

        if (yumDuckets > roadInt) {
            // Fully fund roads
            result.roadValue = roadInt;
            yumDuckets -= roadInt;

            if (yumDuckets > fireInt) {
                // Fully fund fire
                result.fireValue = fireInt;
                yumDuckets -= fireInt;

                if (yumDuckets > policeInt) {
                    // Fully fund police (shouldn't happen given outer condition)
                    result.policeValue = policeInt;
                } else {
                    // Partially fund police
                    result.policeValue = yumDuckets;
                    if (yumDuckets > 0 && input.policeFund > 0) {
                        result.policePercent = static_cast<float>(yumDuckets) /
                                               static_cast<float>(input.policeFund);
                    } else {
                        result.policePercent = 0.0f;
                    }
                }
            } else {
                // Partially fund fire, no police
                result.fireValue = yumDuckets;
                result.policeValue = 0;
                result.policePercent = 0.0f;

                if (yumDuckets > 0 && input.fireFund > 0) {
                    result.firePercent = static_cast<float>(yumDuckets) /
                                         static_cast<float>(input.fireFund);
                } else {
                    result.firePercent = 0.0f;
                }
            }
        } else {
            // Partially fund roads only
            result.roadValue = yumDuckets;
            result.fireValue = 0;
            result.policeValue = 0;
            result.firePercent = 0.0f;
            result.policePercent = 0.0f;

            if (yumDuckets > 0 && input.roadFund > 0) {
                result.roadPercent = static_cast<float>(yumDuckets) /
                                     static_cast<float>(input.roadFund);
            } else {
                result.roadPercent = 0.0f;
            }
        }
    } else {
        // Zero funding requested
        result.fireValue = 0;
        result.policeValue = 0;
        result.roadValue = 0;
        result.firePercent = 1.0f;
        result.policePercent = 1.0f;
        result.roadPercent = 1.0f;
    }

    // Determine spending and whether to show budget window
    result.needsBudgetWindow = (!autoBudget || fromMenu);

    // Recalculate total for the final yumDuckets check
    yumDuckets = input.taxFund + input.totalFunds;
    total = fireInt + policeInt + roadInt;

    if (autoBudget && !fromMenu) {
        if (yumDuckets > total) {
            // Auto-budget with sufficient funds
            result.fireSpend = input.fireFund;
            result.policeSpend = input.policeFund;
            result.roadSpend = input.roadFund;
            result.totalSpend = result.fireSpend + result.policeSpend + result.roadSpend;
            result.cashChange = input.taxFund - result.totalSpend;
        } else {
            // Insufficient funds - need to show budget window
            result.needsBudgetWindow = true;
            result.insufficientFunds = true;

            // Emit no money event
            eventBus_.publish(Events::NoMoney{
                total - yumDuckets  // deficit
            });
        }
    } else {
        // Manual budget or from menu
        if (!fromMenu) {
            result.fireSpend = result.fireValue;
            result.policeSpend = result.policeValue;
            result.roadSpend = result.roadValue;
            result.totalSpend = result.fireSpend + result.policeSpend + result.roadSpend;
            result.cashChange = input.taxFund - result.totalSpend;
        }
    }

    // Emit budget updated event
    eventBus_.publish(Events::BudgetUpdated{
        result.roadSpend,
        result.policeSpend,
        result.fireSpend,
        result.roadValue,    // Using value as effect for now
        result.policeValue,
        result.fireValue
    });

    return result;
}

} // namespace MicropolisEngine
