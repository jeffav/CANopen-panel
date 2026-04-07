/**
 * @file Tasks.cpp
 * @brief FreeRTOS task implementations
 *
 * The task bodies here are HAL-agnostic: they call through the
 * OperatorPanel API, which in turn delegates leaf GPIO reads/writes
 * to the (still-stubbed) hardware abstraction in OperatorPanel.cpp.
 *
 * All FreeRTOS-specific calls are guarded by USE_FREERTOS so that the
 * host unit-test build (which does not link FreeRTOS) still compiles.
 */

#include "Tasks.h"
#include "OperatorPanel.hpp"

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

#ifdef USE_CANOPEN
#include "CANopen.h"
extern CO_t* CO;
#endif

/* Global task handles */
static TaskHandles_t g_taskHandles = {nullptr, nullptr, nullptr, nullptr};

/* External reference to operator panel instance (owned by main.cpp) */
extern OperatorPanel* g_operatorPanel;

#ifndef USE_FREERTOS
/* Host-build monotonic time source. Tests can advance this via
 * Tasks_SetTimeMsForTest(); production firmware never uses this path. */
static uint32_t g_hostTimeMs = 0;
extern "C" void Tasks_SetTimeMsForTest(uint32_t t) { g_hostTimeMs = t; }
extern "C" void Tasks_AdvanceTimeMsForTest(uint32_t d) { g_hostTimeMs += d; }
#endif

bool Tasks_Init(TaskHandles_t* handles)
{
    if (!handles) {
        return false;
    }

#ifdef USE_FREERTOS
    BaseType_t result;

    result = xTaskCreate(Task_CanopenProcess, "CANopen",
                         TASK_STACK_SIZE_CANOPEN, NULL,
                         TASK_PRIORITY_CANOPEN,
                         (TaskHandle_t*)&handles->canopenTask);
    if (result != pdPASS) return false;

    result = xTaskCreate(Task_ButtonHandler, "Button",
                         TASK_STACK_SIZE_BUTTON, NULL,
                         TASK_PRIORITY_BUTTON,
                         (TaskHandle_t*)&handles->buttonTask);
    if (result != pdPASS) return false;

    result = xTaskCreate(Task_GpioMonitor, "GPIO",
                         TASK_STACK_SIZE_GPIO, NULL,
                         TASK_PRIORITY_GPIO,
                         (TaskHandle_t*)&handles->gpioTask);
    if (result != pdPASS) return false;

    result = xTaskCreate(Task_LampControl, "Lamp",
                         TASK_STACK_SIZE_LAMP, NULL,
                         TASK_PRIORITY_LAMP,
                         (TaskHandle_t*)&handles->lampTask);
    if (result != pdPASS) return false;
#endif

    g_taskHandles = *handles;
    return true;
}

void Task_CanopenProcess(void* argument)
{
    (void)argument;
#ifdef USE_FREERTOS
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_PERIOD_CANOPEN_MS);

    for (;;) {
#ifdef USE_CANOPEN
        CO_process(CO, false, TASK_PERIOD_CANOPEN_MS * 1000U, NULL);
#endif
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
#endif
}

void Task_ButtonHandler(void* argument)
{
    (void)argument;
#ifdef USE_FREERTOS
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_PERIOD_BUTTON_MS);

    for (;;) {
        uint32_t now = Tasks_GetTimeMs();
        if (g_operatorPanel && g_operatorPanel->processButtons(now)) {
            Tasks_RequestTpdoTransmit(1);
        }
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
#endif
}

void Task_GpioMonitor(void* argument)
{
    (void)argument;
#ifdef USE_FREERTOS
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_PERIOD_GPIO_MS);

    for (;;) {
        uint32_t now = Tasks_GetTimeMs();
        if (g_operatorPanel && g_operatorPanel->processGpioInputs(now)) {
            Tasks_RequestTpdoTransmit(1);
        }
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
#endif
}

void Task_LampControl(void* argument)
{
    (void)argument;
#ifdef USE_FREERTOS
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_PERIOD_LAMP_MS);

    for (;;) {
        uint32_t now = Tasks_GetTimeMs();
        if (g_operatorPanel) {
            g_operatorPanel->processLampOutputs(now);
        }
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
#endif
}

uint32_t Tasks_GetTimeMs(void)
{
#ifdef USE_FREERTOS
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#else
    return g_hostTimeMs;
#endif
}

void Tasks_NotifyCanopenTask(void)
{
#ifdef USE_FREERTOS
    if (g_taskHandles.canopenTask) {
        xTaskNotifyGive((TaskHandle_t)g_taskHandles.canopenTask);
    }
#endif
}

void Tasks_RequestTpdoTransmit(uint8_t tpdoNum)
{
    (void)tpdoNum;
#if defined(USE_CANOPEN)
    if (CO && tpdoNum >= 1 && CO->TPDO[tpdoNum - 1]) {
        CO_TPDOsendRequest(CO->TPDO[tpdoNum - 1]);
    }
#endif
}
