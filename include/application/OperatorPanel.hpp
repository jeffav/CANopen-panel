/**
 * @file OperatorPanel.hpp
 * @brief Main operator panel controller
 *
 * This class orchestrates buttons, lamps, GPIO, and CANopen integration
 * for the operator panel device.
 */

#ifndef OPERATOR_PANEL_HPP
#define OPERATOR_PANEL_HPP

#include "ButtonInput.hpp"
#include "LampOutput.hpp"
#include "OD.h"
#include <stdint.h>

/**
 * @brief GPIO pin configuration
 */
struct GPIOConfig {
    void* port;                     // GPIO port (e.g., GPIOA)
    uint16_t pin;                   // GPIO pin number
    bool activeHigh;                // true if active high, false if active low
};

/**
 * @brief Hardware configuration for operator panel
 */
struct OperatorPanelConfig {
    GPIOConfig buttonPins[3];       // Button GPIO configurations
    GPIOConfig lampPins[2];         // Lamp GPIO configurations
    GPIOConfig gpioPins[5];         // Additional GPIO configurations
    uint8_t numButtons;             // Number of buttons (typically 3)
    uint8_t numLamps;               // Number of lamps (typically 2)
    uint8_t numGpios;               // Number of GPIO inputs (0-5)
    uint32_t buttonDebounceMs;      // Button debounce time
};

/**
 * @brief Main operator panel controller
 *
 * Integrates buttons, lamps, GPIO, and CANopen Object Dictionary.
 * Handles all I/O operations and coordinates with CANopen stack.
 */
class OperatorPanel {
public:
    /**
     * @brief Constructor
     *
     * @param config Hardware configuration
     */
    OperatorPanel(const OperatorPanelConfig& config);

    /**
     * @brief Destructor
     */
    ~OperatorPanel();

    /**
     * @brief Initialize the operator panel
     *
     * Sets up GPIO, buttons, lamps, and Object Dictionary.
     *
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Process button inputs
     *
     * Reads button GPIOs, updates debouncing, and updates OD.
     * Should be called periodically (10-20ms).
     *
     * @param currentTimeMs Current system time
     * @return true if button state changed (triggers TPDO)
     */
    bool processButtons(uint32_t currentTimeMs);

    /**
     * @brief Process GPIO inputs
     *
     * Reads GPIO inputs and updates OD.
     *
     * @param currentTimeMs Current system time
     * @return true if GPIO state changed (triggers TPDO)
     */
    bool processGpioInputs(uint32_t currentTimeMs);

    /**
     * @brief Process lamp outputs
     *
     * Reads lamp states from OD and updates GPIO outputs.
     *
     * @param currentTimeMs Current system time
     */
    void processLampOutputs(uint32_t currentTimeMs);

    /**
     * @brief Get button array
     *
     * @return Pointer to button array
     */
    ButtonArray* getButtons() { return buttons_; }

    /**
     * @brief Get lamp array
     *
     * @return Pointer to lamp array
     */
    LampArray* getLamps() { return lamps_; }

    /**
     * @brief Read raw button GPIO state
     *
     * @param buttonIndex Button index (0-2)
     * @return true if button GPIO is active
     */
    bool readButtonGpio(uint8_t buttonIndex);

    /**
     * @brief Read raw GPIO input state
     *
     * @param gpioIndex GPIO index (0-4)
     * @return true if GPIO is active
     */
    bool readGpioInput(uint8_t gpioIndex);

    /**
     * @brief Write lamp GPIO output
     *
     * @param lampIndex Lamp index (0-1)
     * @param state Output state
     */
    void writeLampGpio(uint8_t lampIndex, bool state);

    /**
     * @brief Set lamp state via OD
     *
     * Updates Object Dictionary which will be reflected in next
     * lamp output processing cycle.
     *
     * @param lampIndex Lamp index (0-1)
     * @param state Lamp state
     */
    void setLamp(uint8_t lampIndex, bool state);

    /**
     * @brief Emergency stop - turn off all outputs
     *
     * Called when device enters error state.
     */
    void emergencyStop();

    /**
     * @brief Run diagnostics
     *
     * Tests all I/O and returns status.
     *
     * @return true if all tests pass
     */
    bool runDiagnostics();

private:
    /**
     * @brief Initialize GPIO hardware
     */
    bool initGpio();

    /**
     * @brief Read all button GPIOs
     *
     * @param rawStates Output array for raw button states
     */
    void readAllButtons(bool* rawStates);

    /**
     * @brief Read all GPIO inputs
     *
     * @return GPIO input bit mask
     */
    uint8_t readAllGpios();

    /**
     * @brief Write all lamp GPIOs
     */
    void writeAllLamps();

    OperatorPanelConfig config_;    // Hardware configuration
    ButtonArray* buttons_;          // Button input manager
    LampArray* lamps_;              // Lamp output manager
    bool initialized_;              // Initialization status
    uint8_t lastGpioState_;         // Last GPIO state for change detection
};

#endif /* OPERATOR_PANEL_HPP */
