/**
 * @file Tasks.h
 * @brief FreeRTOS task definitions for CANopen operator panel
 *
 * Defines the task structure and priorities for the real-time
 * operating system managing the CANopen device.
 */

#ifndef TASKS_H
#define TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* TODO: Include FreeRTOS headers when available */
/* #include "FreeRTOS.h" */
/* #include "task.h" */
/* #include "queue.h" */
/* #include "semphr.h" */

/* Task priorities (higher number = higher priority) */
#define TASK_PRIORITY_CANOPEN       (4)  /* Highest - CANopen processing */
#define TASK_PRIORITY_BUTTON        (3)  /* High - Button input handling */
#define TASK_PRIORITY_GPIO          (3)  /* High - GPIO input monitoring */
#define TASK_PRIORITY_LAMP          (2)  /* Medium - Lamp output control */

/* Task stack sizes (in words, not bytes) */
#define TASK_STACK_SIZE_CANOPEN     (512)
#define TASK_STACK_SIZE_BUTTON      (256)
#define TASK_STACK_SIZE_GPIO        (256)
#define TASK_STACK_SIZE_LAMP        (256)

/* Task periods (in milliseconds) */
#define TASK_PERIOD_CANOPEN_MS      (1)   /* 1ms - CANopen requires fast processing */
#define TASK_PERIOD_BUTTON_MS       (10)  /* 10ms - Button scanning */
#define TASK_PERIOD_GPIO_MS         (20)  /* 20ms - GPIO monitoring */
#define TASK_PERIOD_LAMP_MS         (20)  /* 20ms - Lamp updates */

/**
 * @brief Task handle structure
 *
 * Holds handles to all FreeRTOS tasks.
 */
typedef struct {
    void* canopenTask;      /* CANopen processing task */
    void* buttonTask;       /* Button input task */
    void* gpioTask;         /* GPIO monitoring task */
    void* lampTask;         /* Lamp output task */
} TaskHandles_t;

/**
 * @brief Initialize and create all RTOS tasks
 *
 * Creates the FreeRTOS tasks for the CANopen operator panel.
 * Should be called before starting the RTOS scheduler.
 *
 * @param handles Pointer to task handles structure
 * @return true if all tasks created successfully
 */
bool Tasks_Init(TaskHandles_t* handles);

/**
 * @brief CANopen processing task
 *
 * High-priority task that processes CANopen stack operations.
 * Runs at 1ms intervals to handle CAN messages, SDO, PDO,
 * heartbeat, and NMT operations.
 *
 * @param argument Task parameter (unused)
 */
void Task_CanopenProcess(void* argument);

/**
 * @brief Button input handling task
 *
 * Scans button inputs, performs debouncing, and updates
 * the Object Dictionary. Triggers TPDOs on button changes.
 *
 * @param argument Task parameter (unused)
 */
void Task_ButtonHandler(void* argument);

/**
 * @brief GPIO input monitoring task
 *
 * Monitors GPIO inputs and updates the Object Dictionary.
 * Triggers TPDOs on GPIO state changes.
 *
 * @param argument Task parameter (unused)
 */
void Task_GpioMonitor(void* argument);

/**
 * @brief Lamp output control task
 *
 * Reads lamp states from Object Dictionary (updated by RPDOs)
 * and controls lamp outputs. Handles PWM and blinking patterns.
 *
 * @param argument Task parameter (unused)
 */
void Task_LampControl(void* argument);

/**
 * @brief Get current system time in milliseconds
 *
 * Returns the current RTOS tick count converted to milliseconds.
 * Used for timestamping and timing operations.
 *
 * @return Current time in milliseconds
 */
uint32_t Tasks_GetTimeMs(void);

/**
 * @brief Notify CANopen task of pending work
 *
 * Notifies the CANopen task that there is work to do
 * (e.g., CAN message received, PDO to transmit).
 */
void Tasks_NotifyCanopenTask(void);

/**
 * @brief Request TPDO transmission
 *
 * Signals the CANopen task to transmit a TPDO.
 *
 * @param tpdoNum TPDO number (1-based)
 */
void Tasks_RequestTpdoTransmit(uint8_t tpdoNum);

#ifdef __cplusplus
}
#endif

#endif /* TASKS_H */
