/**
 * @file test_lamp_output.cpp
 * @brief Unit tests for LampOutput class
 *
 * Tests lamp modes, brightness control, and blinking functionality.
 */

#include <gtest/gtest.h>
#include "LampOutput.hpp"
#include "../mocks/mock_gpio.hpp"

class LampOutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockTime.reset();
    }

    MockTime mockTime;
};

/* ========================================================================
 * Constructor and Initialization Tests
 * ======================================================================== */

TEST_F(LampOutputTest, ConstructorInitializesCorrectly) {
    LampOutput lamp(0);
    EXPECT_EQ(lamp.getIndex(), 0);
    EXPECT_FALSE(lamp.isOn());
    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
}

TEST_F(LampOutputTest, InitSetsOffState) {
    LampOutput lamp(0);
    lamp.turnOn();
    lamp.init();

    EXPECT_FALSE(lamp.isOn());
    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
    EXPECT_FALSE(lamp.getOutputState());
}

/* ========================================================================
 * Basic On/Off Tests
 * ======================================================================== */

TEST_F(LampOutputTest, TurnOnSetsOnState) {
    LampOutput lamp(0);
    lamp.init();

    lamp.turnOn();

    EXPECT_TRUE(lamp.isOn());
    EXPECT_EQ(lamp.getMode(), LampMode::ON);
    EXPECT_TRUE(lamp.getOutputState());
}

TEST_F(LampOutputTest, TurnOffSetsOffState) {
    LampOutput lamp(0);
    lamp.init();
    lamp.turnOn();

    lamp.turnOff();

    EXPECT_FALSE(lamp.isOn());
    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
    EXPECT_FALSE(lamp.getOutputState());
}

TEST_F(LampOutputTest, SetStateTrue) {
    LampOutput lamp(0);
    lamp.init();

    lamp.setState(true);

    EXPECT_TRUE(lamp.isOn());
    EXPECT_TRUE(lamp.getOutputState());
}

TEST_F(LampOutputTest, SetStateFalse) {
    LampOutput lamp(0);
    lamp.init();
    lamp.turnOn();

    lamp.setState(false);

    EXPECT_FALSE(lamp.isOn());
    EXPECT_FALSE(lamp.getOutputState());
}

/* ========================================================================
 * Brightness Tests
 * ======================================================================== */

TEST_F(LampOutputTest, TurnOnWithFullBrightness) {
    LampOutput lamp(0);
    lamp.init();

    lamp.turnOn(255);

    EXPECT_EQ(lamp.getBrightness(), 255);
    EXPECT_EQ(lamp.getMode(), LampMode::ON);
}

TEST_F(LampOutputTest, TurnOnWithPartialBrightness) {
    LampOutput lamp(0);
    lamp.init();

    lamp.turnOn(128);

    EXPECT_EQ(lamp.getBrightness(), 128);
    EXPECT_EQ(lamp.getMode(), LampMode::PWM);
    EXPECT_TRUE(lamp.isOn());
}

TEST_F(LampOutputTest, TurnOnWithZeroBrightnessTurnsOff) {
    LampOutput lamp(0);
    lamp.init();

    lamp.turnOn(0);

    EXPECT_FALSE(lamp.isOn());
    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
}

TEST_F(LampOutputTest, SetBrightnessChangesModeAppropriately) {
    LampOutput lamp(0);
    lamp.init();
    lamp.turnOn();

    // Full brightness -> ON mode
    lamp.setBrightness(255);
    EXPECT_EQ(lamp.getMode(), LampMode::ON);

    // Partial brightness -> PWM mode
    lamp.setBrightness(128);
    EXPECT_EQ(lamp.getMode(), LampMode::PWM);

    // Zero brightness -> OFF mode
    lamp.setBrightness(0);
    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
}

/* ========================================================================
 * Blinking Tests
 * ======================================================================== */

TEST_F(LampOutputTest, StartBlinkingSetsBlinkMode) {
    LampOutput lamp(0);
    lamp.init();

    lamp.startBlinking(500, 500);

    EXPECT_EQ(lamp.getMode(), LampMode::BLINK);
    EXPECT_TRUE(lamp.isOn());  // isOn() returns true for BLINK mode
}

TEST_F(LampOutputTest, BlinkingTogglesOutput) {
    LampOutput lamp(0);
    lamp.init();

    lamp.startBlinking(100, 100);  // 100ms on, 100ms off

    // Initially should be on
    mockTime.setTime(0);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());

    // After 50ms, still on
    mockTime.setTime(50);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());

    // After 105ms, should toggle to off
    mockTime.setTime(105);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_FALSE(lamp.getOutputState());

    // After 210ms, should toggle back to on
    mockTime.setTime(210);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());
}

TEST_F(LampOutputTest, BlinkingAsymmetricPattern) {
    LampOutput lamp(0);
    lamp.init();

    lamp.startBlinking(200, 800);  // 200ms on, 800ms off

    mockTime.setTime(0);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());  // On

    mockTime.setTime(205);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_FALSE(lamp.getOutputState());  // Off after 200ms

    mockTime.setTime(1010);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());  // On after 800ms off
}

TEST_F(LampOutputTest, StopBlinkingReturnsToPreviousState) {
    LampOutput lamp(0);
    lamp.init();

    lamp.turnOn(128);  // PWM mode with 50% brightness
    lamp.startBlinking(100, 100);

    mockTime.setTime(0);
    lamp.update(mockTime.getCurrentTimeMs());

    lamp.stopBlinking();

    EXPECT_EQ(lamp.getMode(), LampMode::PWM);
    EXPECT_EQ(lamp.getBrightness(), 128);
    EXPECT_TRUE(lamp.getOutputState());
}

TEST_F(LampOutputTest, StopBlinkingWhenOffStaysOff) {
    LampOutput lamp(0);
    lamp.init();

    lamp.turnOff();
    lamp.startBlinking(100, 100);
    lamp.stopBlinking();

    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
    EXPECT_FALSE(lamp.getOutputState());
}

/* ========================================================================
 * LampArray Tests
 * ======================================================================== */

class LampArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockTime.reset();
    }

    MockTime mockTime;
};

TEST_F(LampArrayTest, ConstructorCreatesLamps) {
    LampArray array(2);

    EXPECT_EQ(array.getNumLamps(), 2);
    EXPECT_NE(array.getLamp(0), nullptr);
    EXPECT_NE(array.getLamp(1), nullptr);
    EXPECT_EQ(array.getLamp(2), nullptr);  // Out of range
}

TEST_F(LampArrayTest, InitInitializesAllLamps) {
    LampArray array(2);
    array.getLamp(0)->turnOn();
    array.getLamp(1)->turnOn();

    array.init();

    EXPECT_FALSE(array.getLamp(0)->isOn());
    EXPECT_FALSE(array.getLamp(1)->isOn());
}

TEST_F(LampArrayTest, SetFromMaskControlsMultipleLamps) {
    LampArray array(2);
    array.init();

    // Turn on lamp 1 only
    array.setFromMask(0x02);

    EXPECT_FALSE(array.getLamp(0)->isOn());
    EXPECT_TRUE(array.getLamp(1)->isOn());

    // Turn on both lamps
    array.setFromMask(0x03);

    EXPECT_TRUE(array.getLamp(0)->isOn());
    EXPECT_TRUE(array.getLamp(1)->isOn());

    // Turn off all lamps
    array.setFromMask(0x00);

    EXPECT_FALSE(array.getLamp(0)->isOn());
    EXPECT_FALSE(array.getLamp(1)->isOn());
}

TEST_F(LampArrayTest, GetOutputMaskReturnsCorrectState) {
    LampArray array(3);
    array.init();

    array.getLamp(0)->turnOn();
    array.getLamp(2)->turnOn();

    uint8_t mask = array.getOutputMask();

    EXPECT_EQ(mask, 0x05);  // Bits 0 and 2 set
}

TEST_F(LampArrayTest, UpdateAllProcessesAllLamps) {
    LampArray array(2);
    array.init();

    // Start blinking on both lamps
    array.getLamp(0)->startBlinking(100, 100);
    array.getLamp(1)->startBlinking(50, 50);

    mockTime.setTime(0);
    array.updateAll(mockTime.getCurrentTimeMs());

    // Both should be on initially
    EXPECT_TRUE(array.getLamp(0)->getOutputState());
    EXPECT_TRUE(array.getLamp(1)->getOutputState());

    // After 105ms, lamp 0 should toggle
    mockTime.setTime(105);
    array.updateAll(mockTime.getCurrentTimeMs());
    EXPECT_FALSE(array.getLamp(0)->getOutputState());

    // Lamp 1 should have toggled twice by now
    EXPECT_TRUE(array.getLamp(1)->getOutputState());
}

TEST_F(LampArrayTest, GetOutputMaskWithBlinking) {
    LampArray array(2);
    array.init();

    array.getLamp(0)->startBlinking(100, 100);
    array.getLamp(1)->turnOn();

    mockTime.setTime(0);
    array.updateAll(mockTime.getCurrentTimeMs());

    // Initially both on
    uint8_t mask = array.getOutputMask();
    EXPECT_EQ(mask, 0x03);

    // After toggle, lamp 0 off
    mockTime.setTime(105);
    array.updateAll(mockTime.getCurrentTimeMs());
    mask = array.getOutputMask();
    EXPECT_EQ(mask, 0x02);  // Only lamp 1
}

/* ========================================================================
 * Edge Cases
 * ======================================================================== */

TEST_F(LampOutputTest, MultipleUpdatesWithNoTimeChange) {
    LampOutput lamp(0);
    lamp.init();
    lamp.startBlinking(100, 100);

    mockTime.setTime(0);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());

    // Update again with same time
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());  // Should not toggle
}

TEST_F(LampOutputTest, VeryFastBlink) {
    LampOutput lamp(0);
    lamp.init();

    lamp.startBlinking(1, 1);  // 1ms on, 1ms off

    mockTime.setTime(0);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());

    mockTime.setTime(2);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_FALSE(lamp.getOutputState());

    mockTime.setTime(4);
    lamp.update(mockTime.getCurrentTimeMs());
    EXPECT_TRUE(lamp.getOutputState());
}

TEST_F(LampOutputTest, ModeTransitions) {
    LampOutput lamp(0);
    lamp.init();

    // OFF -> ON
    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
    lamp.turnOn();
    EXPECT_EQ(lamp.getMode(), LampMode::ON);

    // ON -> PWM
    lamp.setBrightness(128);
    EXPECT_EQ(lamp.getMode(), LampMode::PWM);

    // PWM -> BLINK
    lamp.startBlinking(100, 100);
    EXPECT_EQ(lamp.getMode(), LampMode::BLINK);

    // BLINK -> PWM (via stopBlinking)
    lamp.stopBlinking();
    EXPECT_EQ(lamp.getMode(), LampMode::PWM);

    // PWM -> OFF
    lamp.turnOff();
    EXPECT_EQ(lamp.getMode(), LampMode::OFF);
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
