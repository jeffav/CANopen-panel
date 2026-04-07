# Architecture Documentation

## System Overview

The CANopen Operator Panel is designed as a multi-layered embedded system following modern software engineering practices for safety-critical real-time applications.

## Layer Architecture

```
┌─────────────────────────────────────────────────┐
│         Application Layer (C++)                  │
│  ┌──────────────┐  ┌──────────────┐            │
│  │ ButtonInput  │  │ LampOutput   │            │
│  └──────────────┘  └──────────────┘            │
│         ┌─────────────────────┐                 │
│         │  OperatorPanel      │                 │
│         └─────────────────────┘                 │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│     CANopen Abstraction Layer (C)                │
│  ┌──────────────────────────────────────────┐  │
│  │  Object Dictionary (OD)                  │  │
│  └──────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│     CANopenNode Stack (C)                        │
│  ┌────┐  ┌────┐  ┌────┐  ┌────┐  ┌────┐       │
│  │PDO │  │SDO │  │NMT │  │SYNC│  │HB  │       │
│  └────┘  └────┘  └────┘  └────┘  └────┘       │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│         FreeRTOS (RTOS Layer)                    │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐          │
│  │Task1 │ │Task2 │ │Task3 │ │Task4 │          │
│  └──────┘ └──────┘ └──────┘ └──────┘          │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│       STM32 HAL / Hardware Layer                 │
│  ┌────────┐  ┌────────┐  ┌────────┐           │
│  │  CAN   │  │  GPIO  │  │ Timer  │           │
│  └────────┘  └────────┘  └────────┘           │
└─────────────────────────────────────────────────┘
```

## Design Principles

### 1. Separation of Concerns
- **Application Layer**: Business logic, I/O state management
- **CANopen Layer**: Protocol implementation, network communication
- **RTOS Layer**: Task scheduling, synchronization
- **Hardware Layer**: Direct hardware access

### 2. C/C++ Integration Strategy

**Why C++ for Application Layer:**
- Object-oriented design for hardware abstraction
- Strong typing and compile-time checks
- RAII for resource management
- Modern language features (constexpr, templates)

**Why C for CANopen/RTOS:**
- CANopenNode is a C library
- FreeRTOS is C-based
- Minimal overhead for real-time critical sections
- Wider toolchain compatibility

**Integration Approach:**
```cpp
extern "C" {
    #include "CANopen.h"
    #include "FreeRTOS.h"
}

// C++ application code
class OperatorPanel {
    // Wraps C APIs cleanly
};
```

### 3. Real-Time Guarantees

**Task Priorities (0=lowest, 4=highest):**
- **Priority 4**: CANopen processing (1ms deterministic)
- **Priority 3**: Button/GPIO (10-20ms, user-interactive)
- **Priority 2**: Lamp output (20ms, non-critical)

**Worst-Case Execution Time (WCET) Targets:**
- CANopen task: < 500μs per iteration
- Button task: < 100μs per iteration
- GPIO task: < 50μs per iteration
- Lamp task: < 50μs per iteration

### 4. Thread-Safety Strategy

**Shared Resources:**
- Object Dictionary (OD_RAM)

**Protection Mechanisms:**
- Critical sections for OD access (< 10μs)
- No dynamic allocation in RT tasks
- Lock-free where possible (atomic operations)

**Example:**
```c
// Safe OD update
taskENTER_CRITICAL();
OD_RAM.buttonInputs = newValue;
taskEXIT_CRITICAL();
```

## Component Details

### ButtonInput Class

**Responsibility**: Button debouncing and edge detection

**State Machine:**
```
    RELEASED ──press──> DEBOUNCING ──timeout──> PRESSED
        ↑                    │                       │
        │                    └──release──           │
        └─────────────<─────────────────────<───────┘
                           (via RELEASING)
```

**Key Features:**
- Configurable debounce time (default 20ms)
- Press/release edge detection
- Time-based state machine
- No blocking operations

**Usage Pattern:**
```cpp
ButtonInput button(0, 20);  // Button 0, 20ms debounce
button.init();

// In periodic task:
bool changed = button.update(gpioRead(), currentTime);
if (button.isPressedEdge()) {
    // Handle button press
}
```

### LampOutput Class

**Responsibility**: Lamp control with multiple modes

**Modes:**
- **OFF**: Lamp off
- **ON**: Lamp on (full brightness)
- **PWM**: Brightness control (0-255)
- **BLINK**: Timed on/off pattern

**Features:**
- Non-blocking blinking
- Smooth brightness transitions (future)
- State preservation during mode changes

**Usage Pattern:**
```cpp
LampOutput lamp(0);
lamp.init();

// Direct control
lamp.turnOn(128);  // 50% brightness

// Blinking
lamp.startBlinking(500, 500);  // 500ms on, 500ms off

// In periodic task:
lamp.update(currentTime);  // Updates blink state
```

### OperatorPanel Class

**Responsibility**: System orchestration and GPIO abstraction

**Key Responsibilities:**
1. GPIO configuration and access
2. Button array management
3. Lamp array management
4. Object Dictionary synchronization
5. Hardware abstraction

**Design Pattern**: Facade pattern
- Provides unified interface to subsystems
- Hides hardware details from upper layers
- Centralizes configuration

**Configuration Strategy:**
```cpp
OperatorPanelConfig config = {
    .buttonPins = {...},  // Hardware-specific
    .lampPins = {...},
    .numButtons = 3,
    .buttonDebounceMs = 20
};

OperatorPanel panel(config);
```

### Object Dictionary (OD)

**Design**: C-based structure for CANopenNode compatibility

**Access Patterns:**
- **Read**: Direct member access (fast)
- **Write**: Through helper functions (validation)

**Thread Safety:**
- Updates from RTOS tasks use critical sections
- CANopen stack accesses are interrupt-driven

**Example:**
```c
// Safe update from task
bool changed = OD_UpdateButtonInputs(buttonMask);
if (changed) {
    // Trigger TPDO
}

// Direct read (safe if single-reader)
uint8_t lampState = OD_RAM.lampOutputs;
```

## Task Architecture

### Task 1: CANopen Processing

**Period**: 1ms (1000 Hz)
**Priority**: Highest (4)
**Stack**: 512 words

**Responsibilities:**
- Process CAN receive queue
- Handle SDO requests
- Process PDO transmission queue
- Update heartbeat timer
- Execute NMT state machine

**Execution Flow:**
```c
void Task_CanopenProcess(void* arg) {
    TickType_t lastWake = xTaskGetTickCount();

    while (1) {
        // Process CANopen stack (non-blocking)
        CO_process(CO, false, 1000, NULL);

        // Wait for next 1ms tick
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1));
    }
}
```

### Task 2: Button Handler

**Period**: 10ms (100 Hz)
**Priority**: 3
**Stack**: 256 words

**Execution Flow:**
```c
void Task_ButtonHandler(void* arg) {
    while (1) {
        uint32_t time = Tasks_GetTimeMs();

        // Update buttons
        bool changed = g_operatorPanel->processButtons(time);

        // Trigger TPDO if changed
        if (changed) {
            Tasks_RequestTpdoTransmit(1);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### Task 3: GPIO Monitor

**Period**: 20ms (50 Hz)
**Priority**: 3
**Stack**: 256 words

**Responsibilities:**
- Read GPIO input states
- Detect state changes
- Update Object Dictionary
- Trigger TPDO on changes

### Task 4: Lamp Control

**Period**: 20ms (50 Hz)
**Priority**: 2 (Lowest)
**Stack**: 256 words

**Responsibilities:**
- Read lamp states from OD (set by RPDO)
- Update lamp objects (handle blinking)
- Write GPIO outputs

## CANopen Integration

### PDO Strategy

**TPDO1 (Transmit PDO 1):**
- **Type**: Event-driven (transmission type 254)
- **Trigger**: Button or GPIO state change
- **Mapping**:
  - Byte 0: Button inputs (0x6000)
  - Byte 1: GPIO inputs (0x6100)
- **Inhibit Time**: 100ms (prevents bus flooding)

**RPDO1 (Receive PDO 1):**
- **Type**: Asynchronous (transmission type 255)
- **Trigger**: On reception
- **Mapping**:
  - Byte 0: Lamp outputs (0x6200)

### SDO Access

All OD entries accessible via SDO for configuration:
- Read button states
- Configure interrupt masks
- Read device identity
- Adjust heartbeat time

### Error Handling

**Error Register (0x1001):**
- Bit 0: Generic error
- Bit 1: Current error (not used)
- Bit 2: Voltage error (not used)
- Bit 3: Temperature error (not used)
- Bit 4: Communication error
- Bit 5: Device profile specific
- Bit 7: Manufacturer specific

**EMCY Messages:**
Sent on critical errors:
- CAN bus off
- Heartbeat timeout (if monitoring master)
- Internal software errors

## Memory Management

### Static Allocation Strategy

**Rationale**: Real-time determinism
- No malloc/free in application
- All objects created at initialization
- Fixed-size buffers

**Memory Sections:**
```
Flash (Code):     ~64 KB
RAM (Data):       ~8 KB
  - .data:        ~2 KB (initialized variables)
  - .bss:         ~4 KB (zero-initialized)
  - Heap:         ~2 KB (FreeRTOS only)
  - Stack:        Task stacks (configurable)
```

### Stack Sizing

**Calculation Method:**
1. Measure actual usage with stack painting
2. Add 25% safety margin
3. Round up to 256-word boundary

**Current Allocations:**
- CANopen task: 512 words (2 KB)
- Button task: 256 words (1 KB)
- GPIO task: 256 words (1 KB)
- Lamp task: 256 words (1 KB)
- **Total**: 5 KB for task stacks

## Performance Analysis

### CPU Utilization (Estimated)

At maximum activity:
- CANopen task: 30% (300μs per 1ms)
- Button task: 1% (100μs per 10ms)
- GPIO task: 0.25% (50μs per 20ms)
- Lamp task: 0.25% (50μs per 20ms)
- **Total**: ~32% CPU utilization
- **Idle**: ~68%

### Network Load

**Steady State:**
- Heartbeat: 1 message per second
- TPDO: ~10 messages per second (typical)
- **Total**: ~11 messages/sec, ~88 bytes/sec

**Peak Load:**
- Multiple button presses: ~10 TPDO/sec (with inhibit)
- SDO configuration: Variable
- **Max estimated**: ~50 messages/sec

## Future Enhancements

### Planned Features

1. **PWM Brightness Control**
   - Hardware timer for true PWM
   - Smooth fade transitions
   - Configurable PWM frequency

2. **Advanced Diagnostics**
   - Button press counters
   - Lamp on-time tracking
   - Error event logging

3. **EEPROM Configuration**
   - Store node ID
   - Store PDO mappings
   - Store custom parameters

4. **LSS Implementation**
   - Dynamic node ID assignment
   - Bitrate detection
   - Factory reset capability

5. **Extended GPIO**
   - Analog inputs (ADC)
   - Encoder inputs
   - Additional digital I/Os

### Scalability

**To add a new button:**
1. Update `OperatorPanelConfig` (increase numButtons)
2. Configure GPIO pin in `main.cpp`
3. Update OD if needed (>3 buttons)

**To add a new lamp:**
1. Update `OperatorPanelConfig` (increase numLamps)
2. Configure GPIO pin
3. Update OD (0x6200 bit mapping)

## Testing Strategy

### Unit Tests
- ButtonInput state machine (all transitions)
- LampOutput mode changes
- OD update functions

### Integration Tests
- Button → OD → TPDO flow
- RPDO → OD → Lamp flow
- SDO read/write operations

### Hardware-in-Loop Tests
- Real-time performance validation
- CAN bus conformance testing
- Stress testing (rapid button presses)

## References

- CANopenNode documentation
- FreeRTOS documentation
- STM32F4 reference manual
- CiA 301/401 specifications
