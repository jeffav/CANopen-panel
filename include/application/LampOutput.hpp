/**
 * @file LampOutput.hpp
 * @brief Lamp output controller with PWM and blinking support
 *
 * This class manages lamp outputs with support for simple on/off,
 * PWM brightness control, and blinking patterns.
 */

#ifndef LAMP_OUTPUT_HPP
#define LAMP_OUTPUT_HPP

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Lamp operating modes
 */
enum class LampMode {
    OFF,            // Lamp is off
    ON,             // Lamp is on (full brightness)
    PWM,            // Lamp uses PWM for brightness control
    BLINK           // Lamp is blinking
};

/**
 * @brief Lamp output controller
 *
 * Controls a single lamp output with support for various modes
 * including simple on/off, PWM brightness, and blinking.
 */
class LampOutput {
public:
    /**
     * @brief Constructor
     *
     * @param lampIndex Lamp number (0-1)
     */
    LampOutput(uint8_t lampIndex);

    /**
     * @brief Initialize the lamp output
     */
    void init();

    /**
     * @brief Update lamp state (call periodically)
     *
     * Handles blinking and other time-based operations.
     *
     * @param currentTimeMs Current system time in milliseconds
     */
    void update(uint32_t currentTimeMs);

    /**
     * @brief Turn lamp on
     *
     * @param brightness Brightness level (0-255, default 255 = full)
     */
    void turnOn(uint8_t brightness = 255);

    /**
     * @brief Turn lamp off
     */
    void turnOff();

    /**
     * @brief Set lamp state
     *
     * @param state true = on, false = off
     */
    void setState(bool state);

    /**
     * @brief Set PWM brightness
     *
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness);

    /**
     * @brief Start blinking pattern
     *
     * @param onTimeMs Time lamp is on in milliseconds
     * @param offTimeMs Time lamp is off in milliseconds
     */
    void startBlinking(uint32_t onTimeMs, uint32_t offTimeMs);

    /**
     * @brief Stop blinking
     *
     * Returns lamp to last non-blinking state.
     */
    void stopBlinking();

    /**
     * @brief Check if lamp is on
     *
     * @return true if lamp is on (any mode except OFF)
     */
    bool isOn() const { return mode_ != LampMode::OFF; }

    /**
     * @brief Get current mode
     *
     * @return Current lamp mode
     */
    LampMode getMode() const { return mode_; }

    /**
     * @brief Get lamp index
     *
     * @return Lamp number (0-1)
     */
    uint8_t getIndex() const { return lampIndex_; }

    /**
     * @brief Get current brightness
     *
     * @return Brightness level (0-255)
     */
    uint8_t getBrightness() const { return brightness_; }

    /**
     * @brief Get current output state (for GPIO control)
     *
     * Considers blinking state.
     *
     * @return true if output should be high
     */
    bool getOutputState() const { return outputState_; }

private:
    uint8_t lampIndex_;             // Lamp number (0-1)
    LampMode mode_;                 // Current operating mode
    uint8_t brightness_;            // Brightness level (0-255)
    bool outputState_;              // Current physical output state
    uint32_t blinkOnTimeMs_;        // Blink on time
    uint32_t blinkOffTimeMs_;       // Blink off time
    uint32_t lastBlinkToggleMs_;    // Time of last blink toggle
    bool blinkPhase_;               // Current blink phase (true = on)
};

/**
 * @brief Lamp array manager
 *
 * Manages multiple lamp outputs and provides combined control.
 */
class LampArray {
public:
    /**
     * @brief Constructor
     *
     * @param numLamps Number of lamps (typically 2)
     */
    LampArray(uint8_t numLamps);

    /**
     * @brief Destructor
     */
    ~LampArray();

    /**
     * @brief Initialize all lamps
     */
    void init();

    /**
     * @brief Update all lamps
     *
     * @param currentTimeMs Current system time
     */
    void updateAll(uint32_t currentTimeMs);

    /**
     * @brief Set lamp states from bit mask
     *
     * Updates lamp states based on bit mask from OD.
     *
     * @param lampMask Bit mask of lamp states
     */
    void setFromMask(uint8_t lampMask);

    /**
     * @brief Get lamp output states as bit mask
     *
     * @return 8-bit mask with lamp output states
     */
    uint8_t getOutputMask() const;

    /**
     * @brief Get specific lamp
     *
     * @param index Lamp index
     * @return Pointer to lamp or nullptr if invalid
     */
    LampOutput* getLamp(uint8_t index);

    /**
     * @brief Get number of lamps
     */
    uint8_t getNumLamps() const { return numLamps_; }

private:
    LampOutput** lamps_;            // Array of lamp pointers
    uint8_t numLamps_;              // Number of lamps
};

#endif /* LAMP_OUTPUT_HPP */
