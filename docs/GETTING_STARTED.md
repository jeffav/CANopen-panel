# Getting Started Guide

This guide will help you get the CANopen Operator Panel project up and running on your hardware.

## Quick Start Checklist

- [ ] Hardware prepared (STM32F4 board, CAN transceiver, buttons, LEDs)
- [ ] Development tools installed (ARM GCC, CMake, OpenOCD)
- [ ] Repository cloned and submodules initialized
- [ ] Hardware configuration updated in code
- [ ] Project built successfully
- [ ] Firmware flashed to device
- [ ] CAN network configured
- [ ] Device tested with CANopen master

## Hardware Requirements

### Minimum Requirements

**Microcontroller:**
- STM32F4 series (F407, F405, F411, etc.)
- Flash: ≥128 KB
- RAM: ≥32 KB
- CAN peripheral (bxCAN)

**Peripherals:**
- 3× Button inputs (active low with pull-up)
- 2× LED/Lamp outputs (active high)
- Optional: 3-5× Additional GPIO inputs
- CAN transceiver (MCP2551, TJA1050, or similar)

**Development Tools:**
- ST-Link V2 or V3 programmer/debugger
- USB cable
- CAN bus cable and termination resistors

### Example Hardware Setup

```
STM32F407 Discovery Board:
┌─────────────────────────────────────┐
│  [USER Button] → PA0                │
│  [Button 2]    → PA1                │
│  [Button 3]    → PA2                │
│                                      │
│  [LED 1]       ← PB0                │
│  [LED 2]       ← PB1                │
│                                      │
│  [CAN_TX]      → PD1 ──┬── CAN-H    │
│  [CAN_RX]      ← PD0 ──┴── CAN-L    │
│                                      │
│  [ST-Link]     USB to PC            │
└─────────────────────────────────────┘
```

### CAN Network Topology

```
   Master         Node 1         Node 2    This Device
   (PLC)       (Operator Panel) (Sensor)  (Node ID 10)
     │              │              │            │
  ┌──┴──────────────┴──────────────┴────────────┴──┐
  │          CAN-H                                  │
  │                                                 │
  │          CAN-L                                  │
  └──┬──────────────┬──────────────┬────────────┬──┘
    120Ω                                        120Ω
   (Term.)                                    (Term.)
```

**Important**: Both ends of the CAN bus MUST have 120Ω termination resistors.

## Software Requirements

### Required Tools

1. **ARM GCC Toolchain** (Version 10.3 or later)
   ```bash
   # Ubuntu/Debian
   sudo apt-get install gcc-arm-none-eabi

   # macOS (Homebrew)
   brew install --cask gcc-arm-embedded

   # Windows
   # Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
   ```

2. **CMake** (Version 3.22 or later)
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake

   # macOS
   brew install cmake

   # Windows
   # Download from: https://cmake.org/download/
   ```

3. **OpenOCD** (For flashing and debugging)
   ```bash
   # Ubuntu/Debian
   sudo apt-get install openocd

   # macOS
   brew install openocd

   # Windows
   # Download from: https://gnutoolchains.com/arm-eabi/openocd/
   ```

4. **Git** (For version control and submodules)
   ```bash
   # Should already be installed on most systems
   git --version
   ```

### Recommended Tools

- **VS Code** with extensions:
  - C/C++ (Microsoft)
  - CMake Tools
  - Cortex-Debug
- **CANopen Configuration Tool**: CANopenNode's Object Dictionary Editor
- **CAN Bus Analyzer**: PCAN-View, Kvaser CanKing, or SocketCAN (Linux)

## Installation Steps

### 1. Clone Repository

```bash
git clone https://github.com/yourusername/CANopen-panel.git
cd CANopen-panel
```

### 2. Initialize Submodules

```bash
git submodule update --init --recursive
```

This will download:
- CANopenNode (CANopen stack)
- CanOpenSTM32 (STM32 drivers)

### 3. Generate STM32 HAL Code (First Time)

Use STM32CubeMX to generate initialization code:

1. Open STM32CubeMX
2. Select your MCU (e.g., STM32F407VG)
3. Configure:
   - **System Core > RCC**: External crystal (HSE)
   - **System Core > SYS**: Debug Serial Wire
   - **Connectivity > CAN1**: Activated, 250 kbit/s
   - **Middleware > FreeRTOS**: CMSIS_V2
   - **GPIO**: Configure button and lamp pins
4. Project Manager:
   - Project Name: CANopen-Panel
   - Toolchain: Makefile
   - Generate code

Copy generated files:
```bash
cp -r Src/* src/hal/
cp -r Inc/* include/hal/
cp Startup/* src/hal/
```

### 4. Configure Hardware Pins

Edit `src/main.cpp` function `createHardwareConfig()`:

```cpp
static OperatorPanelConfig createHardwareConfig()
{
    OperatorPanelConfig config = {};

    // Configure your actual pin assignments
    config.buttonPins[0] = {GPIOA, GPIO_PIN_0, false};  // Button 1 on PA0
    config.buttonPins[1] = {GPIOA, GPIO_PIN_1, false};  // Button 2 on PA1
    config.buttonPins[2] = {GPIOA, GPIO_PIN_2, false};  // Button 3 on PA2

    config.lampPins[0] = {GPIOB, GPIO_PIN_0, true};     // Lamp 1 on PB0
    config.lampPins[1] = {GPIOB, GPIO_PIN_1, true};     // Lamp 2 on PB1

    config.numButtons = 3;
    config.numLamps = 2;
    config.numGpios = 0;  // Optional GPIOs
    config.buttonDebounceMs = 20;

    return config;
}
```

### 5. Add Linker Script

Copy the linker script for your MCU to `config/`:

```bash
# From STM32CubeMX generated project
cp STM32F407VGTx_FLASH.ld config/
```

Or find it in the STM32 CMSIS device directory.

### 6. Build Project

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

Expected output:
```
[100%] Linking CXX executable CANopen-Panel.elf
   text    data     bss     dec     hex filename
  45678    1234    5678   52590    cd6e CANopen-Panel.elf
[100%] Built target CANopen-Panel.elf
```

### 7. Flash Firmware

```bash
# Using OpenOCD (ST-Link)
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program CANopen-Panel.elf verify reset exit"

# Or using st-flash
st-flash write CANopen-Panel.bin 0x08000000
```

## Configuration

### Setting Node ID

Default node ID is 10. To change:

**Option 1**: Edit `src/main.cpp`:
```cpp
#define CO_NODE_ID_DEFAULT  15  // Change to your node ID
```

**Option 2**: Use LSS (Layer Setting Services) at runtime from CANopen master.

### CANopen Bitrate

Default is 250 kbit/s. To change, edit `src/main.cpp`:
```cpp
#define CO_BITRATE_DEFAULT  500  // 500 kbit/s
```

Supported bitrates: 10, 20, 50, 125, 250, 500, 800, 1000 kbit/s

### Debounce Time

Adjust button debounce time in `src/main.cpp`:
```cpp
config.buttonDebounceMs = 20;  // 10-50ms typical range
```

## Testing

### 1. Hardware Test (No Network)

Create a simple test to verify GPIO:

```cpp
// In main(), before starting RTOS:
while (1) {
    // Read button
    bool btn = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    // Control LED
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, btn ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_Delay(100);
}
```

Expected: LED follows button state.

### 2. CANopen Network Test

**Equipment Needed:**
- CAN bus analyzer or another CANopen device
- CANopen master or configuration tool

**Test Procedure:**

1. **Verify Heartbeat:**
   - Monitor CAN bus
   - Look for heartbeat message: COB-ID `0x700 + 10 = 0x70A`
   - Should transmit every 1 second
   - Data: `[0x05]` (operational state)

2. **Test Button Input (SDO Read):**
   ```
   SDO Read Request:
     COB-ID: 0x600 + 10 = 0x60A
     Data: [0x40, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00]
           (Read 0x6000 subindex 0)

   Expected Response:
     COB-ID: 0x580 + 10 = 0x58A
     Data: [0x4F, 0x00, 0x60, 0x00, 0xXX, 0x00, 0x00, 0x00]
           Where 0xXX = button state (0-7)
   ```

3. **Test Lamp Output (SDO Write):**
   ```
   SDO Write Request:
     COB-ID: 0x600 + 10 = 0x60A
     Data: [0x2F, 0x00, 0x62, 0x00, 0x03, 0x00, 0x00, 0x00]
           (Write 0x03 to 0x6200 subindex 0)

   Expected: Both lamps turn on
   ```

4. **Test TPDO (Event-driven):**
   - Press a button
   - Observe TPDO transmission:
     ```
     COB-ID: 0x180 + 10 = 0x18A
     Data: [0xXX, 0xYY]
           0xXX = button states
           0xYY = GPIO states
     ```

5. **Test RPDO (Lamp Control):**
   ```
   Send PDO:
     COB-ID: 0x200 + 10 = 0x20A
     Data: [0x02]  // Turn on lamp 2

   Expected: Lamp 2 turns on
   ```

## Troubleshooting

### Build Issues

**Error**: `arm-none-eabi-gcc: command not found`
- **Solution**: Install ARM GCC toolchain, ensure it's in PATH

**Error**: `Linker script not found`
- **Solution**: Copy linker script to `config/` directory

**Error**: `HAL header files not found`
- **Solution**: Generate HAL code with STM32CubeMX, copy to project

### Runtime Issues

**Device not responding on CAN bus**
1. Check CAN transceiver connections (CANH, CANL)
2. Verify termination resistors (120Ω at each end)
3. Check bitrate matches network
4. Use oscilloscope to verify CAN signals

**Buttons not working**
1. Verify GPIO configuration in `main.cpp`
2. Check pull-up/pull-down settings
3. Adjust debounce time
4. Test with hardware test loop

**Lamps not working**
1. Check GPIO output configuration
2. Verify active-high/active-low setting
3. Test with simple GPIO write
4. Check for sufficient drive current

**Heartbeat not transmitted**
1. Verify CANopen task is running
2. Check CAN peripheral initialization
3. Review FreeRTOS task priorities
4. Use debugger to step through code

### Debug Tips

**Enable Debug Output:**
- Configure UART for printf debugging
- Use SWO (Serial Wire Output) for trace

**Use Debugger:**
```json
// .vscode/launch.json
{
    "type": "cortex-debug",
    "request": "launch",
    "executable": "./build/CANopen-Panel.elf",
    "servertype": "openocd",
    "configFiles": [
        "interface/stlink.cfg",
        "target/stm32f4x.cfg"
    ]
}
```

**Monitor Stack Usage:**
- FreeRTOS has stack overflow checking
- Enable `configCHECK_FOR_STACK_OVERFLOW` in FreeRTOSConfig.h

## Next Steps

Once basic functionality is verified:

1. **Optimize**: Profile code, optimize timing
2. **Extend**: Add more I/O, implement custom features
3. **Document**: Create user manual for your specific hardware
4. **Test**: Perform comprehensive system testing
5. **Deploy**: Flash to production devices

## Getting Help

- **Documentation**: See `docs/` directory
- **Examples**: Check `examples/` (if available)
- **Issues**: Open GitHub issue
- **Community**: CANopenNode forum, STM32 community

## Additional Resources

- [CANopenNode Documentation](https://canopennode.github.io/)
- [CiA 301 Specification](https://www.can-cia.org/)
- [STM32F4 Resources](https://www.st.com/en/microcontrollers-microprocessors/stm32f4-series.html)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
