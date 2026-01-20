# CANopen Operator Panel

A professional CANopen I/O device implementation for STM32F4 microcontrollers using CANopenNode stack, FreeRTOS, and modern C++.

## Overview

This project implements a CANopen CiA 401 digital I/O device (operator panel) featuring:
- **3 Button Inputs** with hardware debouncing
- **2 Lamp Outputs** with PWM and blinking support
- **3-5 GPIO Inputs** for additional monitoring
- **Event-driven TPDOs** for instant button/GPIO change notification
- **RPDOs** for remote lamp control
- **FreeRTOS** multi-tasking architecture
- **Modern C++17** hardware abstraction layer
- **CANopenNode** integration with CiA 301/401 compliance

## Features

### CANopen Capabilities
- ✅ SDO Server for configuration
- ✅ Event-driven TPDO transmission on input changes
- ✅ RPDO reception for output control
- ✅ NMT state machine
- ✅ Heartbeat producer
- ✅ EMCY (Emergency) message support
- ✅ SYNC consumer capability
- ✅ LSS (Layer Setting Services) support

### Hardware Abstraction
- Object-oriented C++ classes for I/O management
- Software button debouncing with state machines
- Configurable active-high/active-low logic
- PWM brightness control for lamps
- Blinking pattern support

### Real-Time Architecture
- 4 FreeRTOS tasks with priority management
- 1ms CANopen processing tick
- 10ms button scanning
- 20ms GPIO monitoring and lamp updates

## Project Structure

```
CANopen-panel/
├── src/
│   ├── application/       # Application logic
│   │   ├── ButtonInput.cpp
│   │   ├── LampOutput.cpp
│   │   ├── OperatorPanel.cpp
│   │   └── Tasks.c
│   ├── canopen/          # CANopen Object Dictionary
│   │   └── OD.c
│   ├── drivers/          # Hardware drivers
│   ├── hal/              # STM32 HAL integration
│   └── main.cpp          # Application entry point
├── include/
│   ├── application/       # Application headers
│   ├── canopen/          # CANopen headers
│   ├── drivers/          # Driver headers
│   └── hal/              # HAL headers
├── lib/
│   ├── CANopenNode/      # CANopenNode stack (submodule)
│   └── CanOpenSTM32/     # STM32 drivers (submodule)
├── config/
│   ├── OperatorPanel.eds # Object Dictionary EDS file
│   └── STM32F407VGTx_FLASH.ld (to be added)
├── docs/                 # Documentation
├── tests/                # Unit tests
└── CMakeLists.txt        # Build configuration
```

## Object Dictionary

The device implements a CiA 401 digital I/O profile with the following key objects:

| Index  | Name                | Type    | Access | Description |
|--------|---------------------|---------|--------|-------------|
| 0x1000 | Device Type         | UINT32  | RO     | 0x00020192 (Digital I/O) |
| 0x1001 | Error Register      | UINT8   | RO     | Error status |
| 0x1017 | Heartbeat Time      | UINT16  | RW     | Heartbeat interval (ms) |
| 0x1018 | Identity Object     | Record  | RO     | Device identification |
| 0x6000 | Button Inputs       | UINT8   | RO     | 3-bit button states |
| 0x6100 | GPIO Inputs         | UINT8   | RO     | 3-5 bit GPIO states |
| 0x6200 | Lamp Outputs        | UINT8   | RW     | 2-bit lamp states |
| 0x6300 | Input Interrupt Mask| UINT8   | RW     | TPDO trigger mask |

### PDO Mapping

**TPDO1** (Event-driven, COB-ID 0x180 + Node ID):
- Transmits on any button or GPIO change
- Maps: 0x6000 (buttons) + 0x6100 (GPIO)
- Inhibit time: 100ms (prevents flooding)

**RPDO1** (Asynchronous, COB-ID 0x200 + Node ID):
- Receives lamp control commands
- Maps: 0x6200 (lamp outputs)

## Building the Project

### Prerequisites

- ARM GCC Toolchain (`arm-none-eabi-gcc`)
- CMake 3.22 or later
- Git (for submodules)
- OpenOCD (for flashing/debugging)
- VS Code with CMake Tools extension (recommended)

### Clone and Initialize

```bash
git clone <repository-url>
cd CANopen-panel
git submodule update --init --recursive
```

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure (Debug build)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Or for Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build .

# Output files will be:
# - CANopen-Panel.elf (ELF executable)
# - CANopen-Panel.hex (Intel HEX format)
# - CANopen-Panel.bin (Binary format)
```

### VS Code Integration

1. Open project in VS Code
2. Install "CMake Tools" extension
3. Select ARM GCC toolchain when prompted
4. Use CMake: Configure and CMake: Build commands
5. Configure `launch.json` for OpenOCD debugging

## Hardware Configuration

### Pin Assignments (Example for STM32F407)

Modify `main.cpp` `createHardwareConfig()` function for your hardware:

```cpp
// Buttons (active low with pull-up)
config.buttonPins[0] = {GPIOA, GPIO_PIN_0, false};
config.buttonPins[1] = {GPIOA, GPIO_PIN_1, false};
config.buttonPins[2] = {GPIOA, GPIO_PIN_2, false};

// Lamps (active high)
config.lampPins[0] = {GPIOB, GPIO_PIN_0, true};
config.lampPins[1] = {GPIOB, GPIO_PIN_1, true};

// GPIO Inputs
config.gpioPins[0] = {GPIOC, GPIO_PIN_0, true};
// ... configure remaining GPIOs
```

### CAN Connection

- **CAN_RX**: PD0 (or per your board)
- **CAN_TX**: PD1 (or per your board)
- **Termination**: 120Ω resistor required at bus ends
- **Bitrate**: Default 250 kbit/s (configurable)

## Configuration

### CANopen Node ID

Set in `main.cpp`:
```cpp
#define CO_NODE_ID_DEFAULT  10  // Change to desired node ID
```

Or configure via LSS at runtime.

### Debounce Time

Set in `main.cpp`:
```cpp
config.buttonDebounceMs = 20;  // Adjust 10-50ms typical
```

### Task Priorities and Periods

Adjust in `include/application/Tasks.h` if needed:
```c
#define TASK_PERIOD_CANOPEN_MS  (1)   // CANopen tick
#define TASK_PERIOD_BUTTON_MS   (10)  // Button scan rate
```

## Usage Examples

### Reading Buttons via SDO

From CANopen master:
```
SDO Read: Index 0x6000, Subindex 0
Returns: 0x07 (all 3 buttons pressed)
```

### Controlling Lamps via SDO

```
SDO Write: Index 0x6200, Subindex 0, Value 0x03
Result: Both lamps turn on
```

### Controlling Lamps via PDO

Configure RPDO1 on master, then send:
```
COB-ID: 0x200 + NodeID
Data: [0x03] (lamp1=ON, lamp2=ON)
```

### Receiving Button Events via PDO

Master receives TPDO1 when buttons change:
```
COB-ID: 0x180 + NodeID
Data: [buttons, gpio_inputs]
```

## FreeRTOS Task Architecture

```
┌─────────────────────────────────────┐
│  Task_CanopenProcess (1ms, Pri=4)  │  ← Highest priority
│  - Process CAN messages             │
│  - Handle SDO/PDO/NMT/Heartbeat    │
└─────────────────────────────────────┘
           ↕ (Notifies)
┌─────────────────────────────────────┐
│  Task_ButtonHandler (10ms, Pri=3)  │
│  - Scan and debounce buttons       │
│  - Update OD, trigger TPDO         │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  Task_GpioMonitor (20ms, Pri=3)    │
│  - Monitor GPIO inputs             │
│  - Update OD, trigger TPDO         │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  Task_LampControl (20ms, Pri=2)    │
│  - Read OD (updated by RPDO)       │
│  - Update lamp GPIOs               │
└─────────────────────────────────────┘
```

## Next Steps

### To Complete the Implementation:

1. **Add STM32 HAL Code**
   - Generate initial code with STM32CubeMX
   - Integrate HAL drivers into `src/hal/`
   - Implement GPIO read/write in `OperatorPanel.cpp`

2. **Complete CANopen Integration**
   - Link Object Dictionary to CANopenNode
   - Implement CANopen callbacks
   - Configure CAN hardware driver

3. **Add Linker Script**
   - Create `config/STM32F407VGTx_FLASH.ld`
   - Or generate with STM32CubeMX

4. **Implement FreeRTOS Tasks**
   - Uncomment TODO sections in `Tasks.c`
   - Add FreeRTOS configuration header

5. **Testing**
   - Unit tests for ButtonInput debouncing
   - Integration tests with CANopen master
   - Hardware-in-the-loop testing

## Debugging

### OpenOCD Configuration

Create `.vscode/launch.json`:
```json
{
  "type": "cortex-debug",
  "request": "launch",
  "servertype": "openocd",
  "cwd": "${workspaceRoot}",
  "executable": "./build/CANopen-Panel.elf",
  "configFiles": ["interface/stlink.cfg", "target/stm32f4x.cfg"]
}
```

### Troubleshooting

- **CAN messages not received**: Check termination resistors, bitrate configuration
- **Buttons not responding**: Verify GPIO configuration, debounce time
- **Lamps not working**: Check RPDO mapping, GPIO output configuration
- **Heartbeat timeout**: Verify CANopen processing task is running at 1ms

## Contributing

Contributions welcome! Please ensure:
- Code follows existing style
- All classes have Doxygen documentation
- Changes are tested on hardware
- Commit messages are descriptive

## License

This project is licensed under Apache 2.0 (compatible with CANopenNode).

## References

- [CANopenNode](https://github.com/CANopenNode/CANopenNode) - CANopen stack
- [CiA 301](https://www.can-cia.org/can-knowledge/canopen/cia301/) - CANopen application layer
- [CiA 401](https://www.can-cia.org/can-knowledge/canopen/cia401/) - CANopen device profile for I/O modules
- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

## Support

For questions or issues, please open an issue on GitHub.