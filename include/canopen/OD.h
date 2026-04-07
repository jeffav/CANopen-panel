/**
 * @file OD.h
 * @brief CANopen Object Dictionary for Operator Panel
 *
 * This file defines the Object Dictionary structure for the CANopen
 * operator panel device with buttons, lamps, and GPIO.
 */

#ifndef OD_H
#define OD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Object Dictionary indexes */
#define OD_H1000_DEV_TYPE              0x1000U
#define OD_H1001_ERR_REG               0x1001U
#define OD_H1005_COBID_SYNC            0x1005U
#define OD_H1014_COBID_EMERGENCY       0x1014U
#define OD_H1015_INHIBIT_TIME_EMCY     0x1015U
#define OD_H1016_CONSUMER_HB_TIME      0x1016U
#define OD_H1017_PRODUCER_HB_TIME      0x1017U
#define OD_H1018_IDENTITY              0x1018U

/* RPDO Communication Parameters */
#define OD_H1400_RXPDO_1_PARAM         0x1400U

/* RPDO Mapping Parameters */
#define OD_H1600_RXPDO_1_MAPPING       0x1600U

/* TPDO Communication Parameters */
#define OD_H1800_TXPDO_1_PARAM         0x1800U

/* TPDO Mapping Parameters */
#define OD_H1A00_TXPDO_1_MAPPING       0x1A00U

/* Manufacturer-specific objects */
#define OD_H6000_BUTTON_INPUTS         0x6000U
#define OD_H6100_GPIO_INPUTS           0x6100U
#define OD_H6200_LAMP_OUTPUTS          0x6200U
#define OD_H6300_INPUT_INTERRUPT_MASK  0x6300U

/* Device Type - Digital I/O Device (CiA 401) */
#define OD_DEV_TYPE                    0x00020192UL

/* Product identification */
#define OD_VENDOR_ID                   0x00000000UL
#define OD_PRODUCT_CODE                0x00000001UL
#define OD_REVISION_NUMBER             0x00010000UL
#define OD_SERIAL_NUMBER               0x00000000UL

/* Button bit masks (0x6000) */
#define OD_BUTTON_1_MASK               (1U << 0)
#define OD_BUTTON_2_MASK               (1U << 1)
#define OD_BUTTON_3_MASK               (1U << 2)

/* Lamp bit masks (0x6200) */
#define OD_LAMP_1_MASK                 (1U << 0)
#define OD_LAMP_2_MASK                 (1U << 1)

/* GPIO bit masks (0x6100) - adjust based on actual hardware */
#define OD_GPIO_1_MASK                 (1U << 0)
#define OD_GPIO_2_MASK                 (1U << 1)
#define OD_GPIO_3_MASK                 (1U << 2)
#define OD_GPIO_4_MASK                 (1U << 3)
#define OD_GPIO_5_MASK                 (1U << 4)

/**
 * @brief Object Dictionary structure
 *
 * This structure holds all the CANopen Object Dictionary entries
 * that are accessible via SDO and mappable to PDOs.
 */
typedef struct {
    /* Standard CANopen objects */
    uint32_t deviceType;           /* 0x1000 - Device Type */
    uint8_t  errorRegister;        /* 0x1001 - Error Register */
    uint16_t producerHeartbeatTime;/* 0x1017 - Heartbeat interval in ms */

    /* Identity object (0x1018) */
    struct {
        uint32_t vendorId;
        uint32_t productCode;
        uint32_t revisionNumber;
        uint32_t serialNumber;
    } identity;

    /* Digital I/O objects */
    uint8_t buttonInputs;          /* 0x6000 - Button states (3 bits) */
    uint8_t gpioInputs;            /* 0x6100 - GPIO input states (3-5 bits) */
    uint8_t lampOutputs;           /* 0x6200 - Lamp output states (2 bits) */
    uint8_t inputInterruptMask;    /* 0x6300 - Interrupt mask for inputs */

} OD_t;

/**
 * @brief Global Object Dictionary instance
 *
 * This is the actual Object Dictionary that will be accessed
 * by the CANopen stack and the application.
 */
extern OD_t OD_RAM;

/**
 * @brief Initialize the Object Dictionary
 *
 * Sets up default values for all OD entries.
 */
void OD_Init(void);

/**
 * @brief Update button input state in OD
 *
 * @param buttonMask Bit mask of button states
 * @return true if value changed (triggers TPDO)
 */
bool OD_UpdateButtonInputs(uint8_t buttonMask);

/**
 * @brief Update GPIO input state in OD
 *
 * @param gpioMask Bit mask of GPIO input states
 * @return true if value changed (triggers TPDO)
 */
bool OD_UpdateGpioInputs(uint8_t gpioMask);

/**
 * @brief Read lamp output state from OD
 *
 * @return Current lamp output states
 */
uint8_t OD_ReadLampOutputs(void);

/**
 * @brief Read specific button state
 *
 * @param buttonNum Button number (0-2)
 * @return true if button is pressed
 */
bool OD_IsButtonPressed(uint8_t buttonNum);

/**
 * @brief Set lamp state in OD
 *
 * @param lampNum Lamp number (0-1)
 * @param state true = on, false = off
 */
void OD_SetLamp(uint8_t lampNum, bool state);

#ifdef __cplusplus
}
#endif

#endif /* OD_H */
