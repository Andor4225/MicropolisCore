/**
 * @file ISubsystem.h
 * @brief Base interface for all game engine subsystems.
 *
 * MODERNIZATION (Phase 3):
 * All extracted subsystems implement this interface to provide
 * consistent lifecycle management and identification.
 */

#ifndef MICROPOLIS_ISUBSYSTEM_H
#define MICROPOLIS_ISUBSYSTEM_H

#include <string>

namespace MicropolisEngine {

/**
 * @brief Base interface for all subsystems.
 *
 * Provides common lifecycle methods and identification for subsystems.
 * Each subsystem manages a specific aspect of the game simulation.
 */
class ISubsystem {
public:
    virtual ~ISubsystem() = default;

    /**
     * @brief Get the name of this subsystem.
     * @return Human-readable subsystem name.
     */
    virtual const char* getName() const = 0;

    /**
     * @brief Initialize the subsystem.
     *
     * Called once when the game starts or a new city is created.
     * Subsystems should reset their state to initial values.
     */
    virtual void initialize() = 0;

    /**
     * @brief Reset the subsystem state.
     *
     * Called when loading a city or resetting the game.
     * Clears any cached state without full reinitialization.
     */
    virtual void reset() = 0;

    /**
     * @brief Check if the subsystem is properly initialized.
     * @return True if ready to process.
     */
    virtual bool isInitialized() const = 0;
};

/**
 * @brief Subsystem that runs on a tick-based schedule.
 *
 * For subsystems that need to be updated each simulation tick
 * (or on a specific phase schedule).
 */
class ITickableSubsystem : public ISubsystem {
public:
    /**
     * @brief Update the subsystem for the current tick.
     * @param simCycle Current simulation cycle (0-1023).
     * @param phase Current simulation phase (0-15).
     */
    virtual void tick(int simCycle, int phase) = 0;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_ISUBSYSTEM_H
