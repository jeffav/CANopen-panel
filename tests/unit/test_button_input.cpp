/**
 * @file test_button_input.cpp
 * @brief Unit tests for ButtonInput class
 *
 * Tests button debouncing, edge detection, and state machine behavior.
 */

#include <gtest/gtest.h>
#include "ButtonInput.hpp"
#include "../mocks/mock_gpio.hpp"

/**
 * @brief Test fixture for ButtonInput tests
 *
 * Provides common setup and utilities for button tests.
 */
class ButtonInputTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockTime.reset();
    }

    void TearDown() override {
        // Cleanup if needed
    }

    MockTime mockTime;
};

/* ========================================================================
 * Constructor and Initialization Tests
 * ======================================================================== */

TEST_F(ButtonInputTest, ConstructorInitializesCorrectly) {
    ButtonInput button(0, 20);
    EXPECT_EQ(button.getIndex(), 0);
    EXPECT_FALSE(button.isPressed());
}

TEST_F(ButtonInputTest, InitSetsPressedStateFalse) {
    ButtonInput button(1, 20);
    button.init();
    EXPECT_FALSE(button.isPressed());
    EXPECT_FALSE(button.isPressedEdge());
    EXPECT_FALSE(button.isReleasedEdge());
}

/* ========================================================================
 * Debouncing Tests - Core Functionality
 * ======================================================================== */

TEST_F(ButtonInputTest, DebounceRejectsShortPulse) {
    ButtonInput button(0, 20);  // 20ms debounce time
    button.init();

    // Simulate a 5ms pulse (too short, should be rejected)
    mockTime.setTime(0);
    bool changed = button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_FALSE(changed);  // Still debouncing, not confirmed

    mockTime.setTime(5);
    changed = button.update(false, mockTime.getCurrentTimeMs());
    EXPECT_FALSE(changed);  // Released before debounce complete

    // Button should not register as pressed
    EXPECT_FALSE(button.isPressed());
    EXPECT_FALSE(button.isPressedEdge());
}

TEST_F(ButtonInputTest, DebounceAcceptsValidPress) {
    ButtonInput button(0, 20);  // 20ms debounce time
    button.init();

    // Simulate valid 25ms press
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());

    mockTime.setTime(25);
    bool changed = button.update(true, mockTime.getCurrentTimeMs());

    // Button should now be confirmed as pressed
    EXPECT_TRUE(changed);
    EXPECT_TRUE(button.isPressed());
}

TEST_F(ButtonInputTest, DebounceAcceptsLongPress) {
    ButtonInput button(0, 20);
    button.init();

    // Press and hold for 100ms
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());

    mockTime.setTime(25);
    bool changed = button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(changed);
    EXPECT_TRUE(button.isPressed());

    // Still pressed after long time
    mockTime.setTime(100);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isPressed());
}

TEST_F(ButtonInputTest, DebounceRejectsGlitch) {
    ButtonInput button(0, 20);
    button.init();

    // Press
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());

    mockTime.setTime(25);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isPressed());

    // Simulate 3ms glitch (bouncing)
    mockTime.setTime(30);
    button.update(false, mockTime.getCurrentTimeMs());

    mockTime.setTime(33);
    button.update(true, mockTime.getCurrentTimeMs());

    // Should still be pressed (glitch rejected)
    EXPECT_TRUE(button.isPressed());
}

/* ========================================================================
 * Edge Detection Tests
 * ======================================================================== */

TEST_F(ButtonInputTest, PressedEdgeDetectedOnce) {
    ButtonInput button(0, 20);
    button.init();

    // Complete a valid press
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());
    mockTime.setTime(25);
    button.update(true, mockTime.getCurrentTimeMs());

    // First check should return true
    EXPECT_TRUE(button.isPressedEdge());

    // Second check should return false (edge cleared)
    EXPECT_FALSE(button.isPressedEdge());
}

TEST_F(ButtonInputTest, ReleasedEdgeDetectedOnce) {
    ButtonInput button(0, 20);
    button.init();

    // Press button
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());
    mockTime.setTime(25);
    button.update(true, mockTime.getCurrentTimeMs());

    // Clear press edge
    button.isPressedEdge();

    // Release button
    mockTime.setTime(50);
    button.update(false, mockTime.getCurrentTimeMs());
    mockTime.setTime(75);
    button.update(false, mockTime.getCurrentTimeMs());

    // First check should return true
    EXPECT_TRUE(button.isReleasedEdge());

    // Second check should return false (edge cleared)
    EXPECT_FALSE(button.isReleasedEdge());
}

TEST_F(ButtonInputTest, MultiplePressesTriggerMultipleEdges) {
    ButtonInput button(0, 20);
    button.init();

    // First press
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());
    mockTime.setTime(25);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isPressedEdge());

    // Release
    mockTime.setTime(50);
    button.update(false, mockTime.getCurrentTimeMs());
    mockTime.setTime(75);
    button.update(false, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isReleasedEdge());

    // Second press
    mockTime.setTime(100);
    button.update(true, mockTime.getCurrentTimeMs());
    mockTime.setTime(125);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isPressedEdge());  // Should trigger again
}

/* ========================================================================
 * State Machine Tests
 * ======================================================================== */

TEST_F(ButtonInputTest, StateMachineReleasedToPressed) {
    ButtonInput button(0, 20);
    button.init();

    // Start: RELEASED
    EXPECT_FALSE(button.isPressed());

    // Press (transition to DEBOUNCING)
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_FALSE(button.isPressed());  // Still debouncing

    // Wait for debounce (transition to PRESSED)
    mockTime.setTime(25);
    bool changed = button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(changed);
    EXPECT_TRUE(button.isPressed());
}

TEST_F(ButtonInputTest, StateMachinePressedToReleased) {
    ButtonInput button(0, 20);
    button.init();

    // Get to PRESSED state
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());
    mockTime.setTime(25);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isPressed());

    // Release (transition to RELEASING)
    mockTime.setTime(50);
    button.update(false, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isPressed());  // Still debouncing release

    // Wait for debounce (transition to RELEASED)
    mockTime.setTime(75);
    bool changed = button.update(false, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(changed);
    EXPECT_FALSE(button.isPressed());
}

/* ========================================================================
 * Bit Mask Tests
 * ======================================================================== */

TEST_F(ButtonInputTest, GetBitMaskWhenPressed) {
    ButtonInput button0(0, 20);
    ButtonInput button1(1, 20);
    ButtonInput button2(2, 20);

    button0.init();
    button1.init();
    button2.init();

    // Press button 1
    mockTime.setTime(0);
    button1.update(true, mockTime.getCurrentTimeMs());
    mockTime.setTime(25);
    button1.update(true, mockTime.getCurrentTimeMs());

    EXPECT_EQ(button0.getBitMask(), 0x00);
    EXPECT_EQ(button1.getBitMask(), 0x02);  // Bit 1
    EXPECT_EQ(button2.getBitMask(), 0x00);
}

/* ========================================================================
 * ButtonArray Tests
 * ======================================================================== */

class ButtonArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockTime.reset();
    }

    MockTime mockTime;
};

TEST_F(ButtonArrayTest, ConstructorCreatesButtons) {
    ButtonArray array(3, 20);
    EXPECT_EQ(array.getNumButtons(), 3);
    EXPECT_NE(array.getButton(0), nullptr);
    EXPECT_NE(array.getButton(1), nullptr);
    EXPECT_NE(array.getButton(2), nullptr);
    EXPECT_EQ(array.getButton(3), nullptr);  // Out of range
}

TEST_F(ButtonArrayTest, InitInitializesAllButtons) {
    ButtonArray array(3, 20);
    array.init();

    EXPECT_FALSE(array.getButton(0)->isPressed());
    EXPECT_FALSE(array.getButton(1)->isPressed());
    EXPECT_FALSE(array.getButton(2)->isPressed());
}

TEST_F(ButtonArrayTest, UpdateAllProcessesAllButtons) {
    ButtonArray array(3, 20);
    array.init();

    // Press button 0 and button 2
    bool rawStates[] = {true, false, true};

    mockTime.setTime(0);
    array.updateAll(rawStates, mockTime.getCurrentTimeMs());

    mockTime.setTime(25);
    bool changed = array.updateAll(rawStates, mockTime.getCurrentTimeMs());

    EXPECT_TRUE(changed);
    EXPECT_TRUE(array.getButton(0)->isPressed());
    EXPECT_FALSE(array.getButton(1)->isPressed());
    EXPECT_TRUE(array.getButton(2)->isPressed());
}

TEST_F(ButtonArrayTest, GetButtonMaskCombinesAllButtons) {
    ButtonArray array(3, 20);
    array.init();

    // Press all buttons
    bool rawStates[] = {true, true, true};

    mockTime.setTime(0);
    array.updateAll(rawStates, mockTime.getCurrentTimeMs());
    mockTime.setTime(25);
    array.updateAll(rawStates, mockTime.getCurrentTimeMs());

    uint8_t mask = array.getButtonMask();
    EXPECT_EQ(mask, 0x07);  // All 3 bits set
}

TEST_F(ButtonArrayTest, UpdateDetectsChange) {
    ButtonArray array(2, 20);
    array.init();

    // Initially no buttons pressed
    bool rawStates1[] = {false, false};
    mockTime.setTime(0);
    array.updateAll(rawStates1, mockTime.getCurrentTimeMs());

    // Press button 0
    bool rawStates2[] = {true, false};
    mockTime.setTime(10);
    array.updateAll(rawStates2, mockTime.getCurrentTimeMs());

    mockTime.setTime(35);
    bool changed = array.updateAll(rawStates2, mockTime.getCurrentTimeMs());

    EXPECT_TRUE(changed);  // State changed
}

/* ========================================================================
 * Reset Tests
 * ======================================================================== */

TEST_F(ButtonInputTest, ResetClearsState) {
    ButtonInput button(0, 20);
    button.init();

    // Press button
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());
    mockTime.setTime(25);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(button.isPressed());

    // Reset
    button.reset();

    // Should be back to initial state
    EXPECT_FALSE(button.isPressed());
    EXPECT_FALSE(button.isPressedEdge());
}

/* ========================================================================
 * Edge Case Tests
 * ======================================================================== */

TEST_F(ButtonInputTest, ZeroDebounceTime) {
    ButtonInput button(0, 0);  // 0ms debounce
    button.init();

    // Should accept immediately
    mockTime.setTime(0);
    bool changed = button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(changed);
    EXPECT_TRUE(button.isPressed());
}

TEST_F(ButtonInputTest, LongDebounceTime) {
    ButtonInput button(0, 100);  // 100ms debounce
    button.init();

    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());

    // 50ms should not be enough
    mockTime.setTime(50);
    button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_FALSE(button.isPressed());

    // 105ms should be enough
    mockTime.setTime(105);
    bool changed = button.update(true, mockTime.getCurrentTimeMs());
    EXPECT_TRUE(changed);
    EXPECT_TRUE(button.isPressed());
}

TEST_F(ButtonInputTest, RapidPressReleaseCycles) {
    ButtonInput button(0, 20);
    button.init();

    for (int i = 0; i < 5; i++) {
        uint32_t baseTime = i * 100;

        // Press
        mockTime.setTime(baseTime);
        button.update(true, mockTime.getCurrentTimeMs());
        mockTime.setTime(baseTime + 25);
        button.update(true, mockTime.getCurrentTimeMs());
        EXPECT_TRUE(button.isPressed());
        button.isPressedEdge();  // Clear edge

        // Release
        mockTime.setTime(baseTime + 50);
        button.update(false, mockTime.getCurrentTimeMs());
        mockTime.setTime(baseTime + 75);
        button.update(false, mockTime.getCurrentTimeMs());
        EXPECT_FALSE(button.isPressed());
        button.isReleasedEdge();  // Clear edge
    }
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
