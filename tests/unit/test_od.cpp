/**
 * @file test_od.cpp
 * @brief Unit tests for Object Dictionary
 *
 * Tests Object Dictionary initialization, updates, and access functions.
 */

#include <gtest/gtest.h>

extern "C" {
    #include "OD.h"
}

class ObjectDictionaryTest : public ::testing::Test {
protected:
    void SetUp() override {
        OD_Init();
    }
};

/* ========================================================================
 * Initialization Tests
 * ======================================================================== */

TEST_F(ObjectDictionaryTest, InitializesDeviceType) {
    EXPECT_EQ(OD_RAM.deviceType, OD_DEV_TYPE);
    EXPECT_EQ(OD_RAM.deviceType, 0x00020192UL);  // CiA 401 Digital I/O
}

TEST_F(ObjectDictionaryTest, InitializesErrorRegisterToZero) {
    EXPECT_EQ(OD_RAM.errorRegister, 0);
}

TEST_F(ObjectDictionaryTest, InitializesIdentity) {
    EXPECT_EQ(OD_RAM.identity.vendorId, OD_VENDOR_ID);
    EXPECT_EQ(OD_RAM.identity.productCode, OD_PRODUCT_CODE);
    EXPECT_EQ(OD_RAM.identity.revisionNumber, OD_REVISION_NUMBER);
}

TEST_F(ObjectDictionaryTest, InitializesIOToZero) {
    EXPECT_EQ(OD_RAM.buttonInputs, 0);
    EXPECT_EQ(OD_RAM.gpioInputs, 0);
    EXPECT_EQ(OD_RAM.lampOutputs, 0);
}

TEST_F(ObjectDictionaryTest, InitializesInterruptMaskToAll) {
    EXPECT_EQ(OD_RAM.inputInterruptMask, 0xFF);  // All enabled
}

/* ========================================================================
 * Button Input Tests
 * ======================================================================== */

TEST_F(ObjectDictionaryTest, UpdateButtonInputsChangesValue) {
    bool changed = OD_UpdateButtonInputs(0x05);  // Buttons 0 and 2

    EXPECT_TRUE(changed);
    EXPECT_EQ(OD_RAM.buttonInputs, 0x05);
}

TEST_F(ObjectDictionaryTest, UpdateButtonInputsNoChange) {
    OD_UpdateButtonInputs(0x03);
    bool changed = OD_UpdateButtonInputs(0x03);  // Same value

    EXPECT_FALSE(changed);
}

TEST_F(ObjectDictionaryTest, UpdateButtonInputsMasksExtraBits) {
    OD_UpdateButtonInputs(0xFF);  // All bits set

    EXPECT_EQ(OD_RAM.buttonInputs, 0x07);  // Only 3 button bits
}

TEST_F(ObjectDictionaryTest, IsButtonPressedReturnsCorrectly) {
    OD_UpdateButtonInputs(0x05);  // Buttons 0 and 2

    EXPECT_TRUE(OD_IsButtonPressed(0));
    EXPECT_FALSE(OD_IsButtonPressed(1));
    EXPECT_TRUE(OD_IsButtonPressed(2));
}

TEST_F(ObjectDictionaryTest, IsButtonPressedInvalidIndex) {
    OD_UpdateButtonInputs(0xFF);

    EXPECT_FALSE(OD_IsButtonPressed(3));  // Out of range
    EXPECT_FALSE(OD_IsButtonPressed(255));
}

/* ========================================================================
 * GPIO Input Tests
 * ======================================================================== */

TEST_F(ObjectDictionaryTest, UpdateGpioInputsChangesValue) {
    bool changed = OD_UpdateGpioInputs(0x15);  // GPIOs 0, 2, 4

    EXPECT_TRUE(changed);
    EXPECT_EQ(OD_RAM.gpioInputs, 0x15);
}

TEST_F(ObjectDictionaryTest, UpdateGpioInputsNoChange) {
    OD_UpdateGpioInputs(0x0A);
    bool changed = OD_UpdateGpioInputs(0x0A);

    EXPECT_FALSE(changed);
}

TEST_F(ObjectDictionaryTest, UpdateGpioInputsMasksExtraBits) {
    OD_UpdateGpioInputs(0xFF);

    EXPECT_EQ(OD_RAM.gpioInputs, 0x1F);  // Only 5 GPIO bits
}

/* ========================================================================
 * Lamp Output Tests
 * ======================================================================== */

TEST_F(ObjectDictionaryTest, ReadLampOutputsReturnsValue) {
    OD_RAM.lampOutputs = 0x03;

    uint8_t value = OD_ReadLampOutputs();

    EXPECT_EQ(value, 0x03);
}

TEST_F(ObjectDictionaryTest, ReadLampOutputsMasksExtraBits) {
    OD_RAM.lampOutputs = 0xFF;

    uint8_t value = OD_ReadLampOutputs();

    EXPECT_EQ(value, 0x03);  // Only 2 lamp bits
}

TEST_F(ObjectDictionaryTest, SetLampTurnsLampOn) {
    OD_SetLamp(0, true);

    EXPECT_EQ(OD_RAM.lampOutputs, 0x01);
}

TEST_F(ObjectDictionaryTest, SetLampTurnsLampOff) {
    OD_RAM.lampOutputs = 0x03;

    OD_SetLamp(0, false);

    EXPECT_EQ(OD_RAM.lampOutputs, 0x02);  // Lamp 1 still on
}

TEST_F(ObjectDictionaryTest, SetLampMultipleLamps) {
    OD_SetLamp(0, true);
    OD_SetLamp(1, true);

    EXPECT_EQ(OD_RAM.lampOutputs, 0x03);

    OD_SetLamp(1, false);

    EXPECT_EQ(OD_RAM.lampOutputs, 0x01);
}

TEST_F(ObjectDictionaryTest, SetLampInvalidIndex) {
    OD_RAM.lampOutputs = 0x00;

    OD_SetLamp(2, true);  // Invalid index

    EXPECT_EQ(OD_RAM.lampOutputs, 0x00);  // No change
}

/* ========================================================================
 * Combined Scenarios
 * ======================================================================== */

TEST_F(ObjectDictionaryTest, SimulateButtonPressSequence) {
    // Initial state
    EXPECT_EQ(OD_RAM.buttonInputs, 0x00);

    // Press button 0
    bool changed = OD_UpdateButtonInputs(0x01);
    EXPECT_TRUE(changed);
    EXPECT_TRUE(OD_IsButtonPressed(0));

    // Press button 1 as well
    changed = OD_UpdateButtonInputs(0x03);
    EXPECT_TRUE(changed);
    EXPECT_TRUE(OD_IsButtonPressed(0));
    EXPECT_TRUE(OD_IsButtonPressed(1));

    // Release button 0
    changed = OD_UpdateButtonInputs(0x02);
    EXPECT_TRUE(changed);
    EXPECT_FALSE(OD_IsButtonPressed(0));
    EXPECT_TRUE(OD_IsButtonPressed(1));

    // Release all
    changed = OD_UpdateButtonInputs(0x00);
    EXPECT_TRUE(changed);
    EXPECT_FALSE(OD_IsButtonPressed(0));
    EXPECT_FALSE(OD_IsButtonPressed(1));
}

TEST_F(ObjectDictionaryTest, SimulateLampControlSequence) {
    // Turn on lamp 0
    OD_SetLamp(0, true);
    EXPECT_EQ(OD_ReadLampOutputs(), 0x01);

    // Turn on lamp 1
    OD_SetLamp(1, true);
    EXPECT_EQ(OD_ReadLampOutputs(), 0x03);

    // Turn off lamp 0
    OD_SetLamp(0, false);
    EXPECT_EQ(OD_ReadLampOutputs(), 0x02);

    // Turn off lamp 1
    OD_SetLamp(1, false);
    EXPECT_EQ(OD_ReadLampOutputs(), 0x00);
}

TEST_F(ObjectDictionaryTest, MultipleReInitializations) {
    // Modify OD
    OD_UpdateButtonInputs(0x07);
    OD_SetLamp(0, true);
    OD_SetLamp(1, true);
    OD_RAM.errorRegister = 0xFF;

    // Re-initialize
    OD_Init();

    // Should be reset
    EXPECT_EQ(OD_RAM.buttonInputs, 0x00);
    EXPECT_EQ(OD_RAM.lampOutputs, 0x00);
    EXPECT_EQ(OD_RAM.errorRegister, 0x00);
    EXPECT_EQ(OD_RAM.deviceType, OD_DEV_TYPE);  // Constants preserved
}

/* ========================================================================
 * Bit Mask Constants Tests
 * ======================================================================== */

TEST_F(ObjectDictionaryTest, ButtonMaskConstants) {
    EXPECT_EQ(OD_BUTTON_1_MASK, 0x01);
    EXPECT_EQ(OD_BUTTON_2_MASK, 0x02);
    EXPECT_EQ(OD_BUTTON_3_MASK, 0x04);
}

TEST_F(ObjectDictionaryTest, LampMaskConstants) {
    EXPECT_EQ(OD_LAMP_1_MASK, 0x01);
    EXPECT_EQ(OD_LAMP_2_MASK, 0x02);
}

TEST_F(ObjectDictionaryTest, GpioMaskConstants) {
    EXPECT_EQ(OD_GPIO_1_MASK, 0x01);
    EXPECT_EQ(OD_GPIO_2_MASK, 0x02);
    EXPECT_EQ(OD_GPIO_3_MASK, 0x04);
    EXPECT_EQ(OD_GPIO_4_MASK, 0x08);
    EXPECT_EQ(OD_GPIO_5_MASK, 0x10);
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
