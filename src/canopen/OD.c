/**
 * @file OD.c
 * @brief CANopen Object Dictionary implementation
 */

#include "OD.h"
#include <string.h>

/* Global Object Dictionary instance */
OD_t OD_RAM = {0};

/* Previous values for change detection */
static uint8_t prevButtonInputs = 0;
static uint8_t prevGpioInputs = 0;

void OD_Init(void)
{
    /* Clear the entire OD structure */
    memset(&OD_RAM, 0, sizeof(OD_RAM));

    /* Initialize standard objects */
    OD_RAM.deviceType = OD_DEV_TYPE;
    OD_RAM.errorRegister = 0;
    OD_RAM.producerHeartbeatTime = 1000; /* 1 second default */

    /* Initialize identity object */
    OD_RAM.identity.vendorId = OD_VENDOR_ID;
    OD_RAM.identity.productCode = OD_PRODUCT_CODE;
    OD_RAM.identity.revisionNumber = OD_REVISION_NUMBER;
    OD_RAM.identity.serialNumber = OD_SERIAL_NUMBER;

    /* Initialize I/O objects */
    OD_RAM.buttonInputs = 0;
    OD_RAM.gpioInputs = 0;
    OD_RAM.lampOutputs = 0;
    OD_RAM.inputInterruptMask = 0xFF; /* All inputs enabled by default */

    /* Initialize previous values */
    prevButtonInputs = 0;
    prevGpioInputs = 0;
}

bool OD_UpdateButtonInputs(uint8_t buttonMask)
{
    /* Mask to only valid button bits (3 buttons) */
    buttonMask &= 0x07;

    /* Check if value changed */
    bool changed = (OD_RAM.buttonInputs != buttonMask);

    /* Update OD */
    OD_RAM.buttonInputs = buttonMask;
    prevButtonInputs = buttonMask;

    return changed;
}

bool OD_UpdateGpioInputs(uint8_t gpioMask)
{
    /* Mask to valid GPIO bits (5 bits max) */
    gpioMask &= 0x1F;

    /* Check if value changed */
    bool changed = (OD_RAM.gpioInputs != gpioMask);

    /* Update OD */
    OD_RAM.gpioInputs = gpioMask;
    prevGpioInputs = gpioMask;

    return changed;
}

uint8_t OD_ReadLampOutputs(void)
{
    /* Return current lamp states (2 lamps) */
    return OD_RAM.lampOutputs & 0x03;
}

bool OD_IsButtonPressed(uint8_t buttonNum)
{
    if (buttonNum > 2) {
        return false;
    }

    return (OD_RAM.buttonInputs & (1U << buttonNum)) != 0;
}

void OD_SetLamp(uint8_t lampNum, bool state)
{
    if (lampNum > 1) {
        return;
    }

    if (state) {
        OD_RAM.lampOutputs |= (1U << lampNum);
    } else {
        OD_RAM.lampOutputs &= ~(1U << lampNum);
    }
}
