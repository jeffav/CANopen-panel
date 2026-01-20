/**
 * @file mock_gpio.hpp
 * @brief Mock GPIO interface for unit testing
 *
 * Provides a controllable GPIO implementation for testing without hardware.
 */

#ifndef MOCK_GPIO_HPP
#define MOCK_GPIO_HPP

#include <map>
#include <cstdint>

/**
 * @brief Mock GPIO interface for testing
 *
 * Simulates GPIO operations without requiring actual hardware.
 * Pin states are stored in memory and can be controlled by tests.
 */
class MockGPIO {
public:
    MockGPIO() = default;

    /**
     * @brief Read a GPIO pin state
     *
     * @param port GPIO port (ignored in mock)
     * @param pin Pin number
     * @return Current pin state
     */
    bool readPin(void* port, uint16_t pin) {
        (void)port;  // Unused in mock
        return pinStates_[pin];
    }

    /**
     * @brief Write a GPIO pin state
     *
     * @param port GPIO port (ignored in mock)
     * @param pin Pin number
     * @param state State to write
     */
    void writePin(void* port, uint16_t pin, bool state) {
        (void)port;  // Unused in mock
        pinStates_[pin] = state;
        writeCount_[pin]++;
    }

    /**
     * @brief Set mock pin state (test helper)
     *
     * Simulates external hardware changing pin state.
     *
     * @param pin Pin number
     * @param state New state
     */
    void setMockPinState(uint16_t pin, bool state) {
        pinStates_[pin] = state;
    }

    /**
     * @brief Get mock pin state (test helper)
     *
     * @param pin Pin number
     * @return Current pin state
     */
    bool getMockPinState(uint16_t pin) const {
        auto it = pinStates_.find(pin);
        return (it != pinStates_.end()) ? it->second : false;
    }

    /**
     * @brief Get write count for a pin (test verification)
     *
     * @param pin Pin number
     * @return Number of times pin was written
     */
    uint32_t getWriteCount(uint16_t pin) const {
        auto it = writeCount_.find(pin);
        return (it != writeCount_.end()) ? it->second : 0;
    }

    /**
     * @brief Reset all pin states
     */
    void reset() {
        pinStates_.clear();
        writeCount_.clear();
    }

    /**
     * @brief Get all pin states (test introspection)
     *
     * @return Map of pin number to state
     */
    const std::map<uint16_t, bool>& getAllPinStates() const {
        return pinStates_;
    }

private:
    std::map<uint16_t, bool> pinStates_;     // Current pin states
    std::map<uint16_t, uint32_t> writeCount_; // Write operation counter
};

/**
 * @brief Mock time provider for testing
 *
 * Provides controllable time for testing time-dependent code.
 */
class MockTime {
public:
    MockTime() : currentTimeMs_(0) {}

    /**
     * @brief Get current mock time
     *
     * @return Current time in milliseconds
     */
    uint32_t getCurrentTimeMs() const {
        return currentTimeMs_;
    }

    /**
     * @brief Advance mock time
     *
     * @param deltaMs Time to advance in milliseconds
     */
    void advanceTime(uint32_t deltaMs) {
        currentTimeMs_ += deltaMs;
    }

    /**
     * @brief Set absolute time
     *
     * @param timeMs New time in milliseconds
     */
    void setTime(uint32_t timeMs) {
        currentTimeMs_ = timeMs;
    }

    /**
     * @brief Reset time to zero
     */
    void reset() {
        currentTimeMs_ = 0;
    }

private:
    uint32_t currentTimeMs_;
};

#endif /* MOCK_GPIO_HPP */
