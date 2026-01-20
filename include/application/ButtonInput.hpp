/**
 * @file ButtonInput.hpp
 * @brief Button input handler with debouncing and edge detection
 *
 * This class manages button inputs with software debouncing,
 * edge detection, and automatic Object Dictionary updates.
 */

#ifndef BUTTON_INPUT_HPP
#define BUTTON_INPUT_HPP

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Button states for state machine
 */
enum class ButtonState {
    RELEASED,       // Button is released
    DEBOUNCING,     // Button press is being debounced
    PRESSED,        // Button is pressed (stable)
    RELEASING       // Button release is being debounced
};

/**
 * @brief Button input handler with debouncing
 *
 * Implements software debouncing using a state machine approach.
 * Detects button press and release edges and maintains button state.
 */
class ButtonInput {
public:
    /**
     * @brief Constructor
     *
     * @param buttonIndex Button number (0-2)
     * @param debounceTimeMs Debounce time in milliseconds (default 20ms)
     */
    ButtonInput(uint8_t buttonIndex, uint32_t debounceTimeMs = 20);

    /**
     * @brief Initialize the button input
     *
     * Sets up internal state and prepares for operation.
     */
    void init();

    /**
     * @brief Update button state (call periodically)
     *
     * Reads the current GPIO state and updates the debouncing
     * state machine. Should be called from a periodic task.
     *
     * @param rawState Current raw GPIO state (true = pressed)
     * @param currentTimeMs Current system time in milliseconds
     * @return true if button state changed
     */
    bool update(bool rawState, uint32_t currentTimeMs);

    /**
     * @brief Check if button is currently pressed
     *
     * @return true if button is in pressed state
     */
    bool isPressed() const { return state_ == ButtonState::PRESSED; }

    /**
     * @brief Check for press edge (button just pressed)
     *
     * Returns true only once per button press.
     *
     * @return true if rising edge detected
     */
    bool isPressedEdge();

    /**
     * @brief Check for release edge (button just released)
     *
     * Returns true only once per button release.
     *
     * @return true if falling edge detected
     */
    bool isReleasedEdge();

    /**
     * @brief Get button index
     *
     * @return Button number (0-2)
     */
    uint8_t getIndex() const { return buttonIndex_; }

    /**
     * @brief Get current button state as bit mask
     *
     * @return Bit mask for this button (1 << buttonIndex)
     */
    uint8_t getBitMask() const;

    /**
     * @brief Reset button state
     *
     * Resets the button to released state.
     */
    void reset();

private:
    uint8_t buttonIndex_;           // Button number (0-2)
    ButtonState state_;             // Current button state
    uint32_t debounceTimeMs_;       // Debounce time in milliseconds
    uint32_t stateChangeTimeMs_;    // Time of last state change
    bool pressEdge_;                // Press edge flag
    bool releaseEdge_;              // Release edge flag
    bool lastRawState_;             // Last raw GPIO reading
};

/**
 * @brief Button array manager
 *
 * Manages multiple button inputs and provides combined status.
 */
class ButtonArray {
public:
    /**
     * @brief Constructor
     *
     * @param numButtons Number of buttons (typically 3)
     * @param debounceTimeMs Debounce time for all buttons
     */
    ButtonArray(uint8_t numButtons, uint32_t debounceTimeMs = 20);

    /**
     * @brief Destructor
     */
    ~ButtonArray();

    /**
     * @brief Initialize all buttons
     */
    void init();

    /**
     * @brief Update all button states
     *
     * @param rawStates Array of raw GPIO states
     * @param currentTimeMs Current system time
     * @return true if any button state changed
     */
    bool updateAll(const bool* rawStates, uint32_t currentTimeMs);

    /**
     * @brief Get combined button state as bit mask
     *
     * @return 8-bit mask with button states
     */
    uint8_t getButtonMask() const;

    /**
     * @brief Get specific button
     *
     * @param index Button index
     * @return Pointer to button or nullptr if invalid
     */
    ButtonInput* getButton(uint8_t index);

    /**
     * @brief Get number of buttons
     */
    uint8_t getNumButtons() const { return numButtons_; }

private:
    ButtonInput** buttons_;         // Array of button pointers
    uint8_t numButtons_;            // Number of buttons
    uint32_t debounceTimeMs_;       // Debounce time
};

#endif /* BUTTON_INPUT_HPP */
