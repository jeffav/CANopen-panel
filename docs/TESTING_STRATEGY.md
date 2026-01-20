# Testing Strategy

## Overview

This document outlines the comprehensive testing strategy for the CANopen Operator Panel project, following the principle: **"You need a way to verify your work."**

## Testing Pyramid

```
                    ┌─────────────────┐
                    │  HIL Testing    │  (Hardware-in-Loop)
                    │   (Manual)      │  (5% of tests)
                    └─────────────────┘
                           ▲
                  ┌────────────────────┐
                  │ Integration Tests  │  (Simulated CANopen)
                  │  (Automated)       │  (15% of tests)
                  └────────────────────┘
                           ▲
              ┌────────────────────────────┐
              │      Unit Tests            │  (Host-based)
              │     (Automated)            │  (80% of tests)
              └────────────────────────────┘
```

## Test Levels

### 1. Unit Tests (Host-Based) ⭐ Primary Focus

**Goal**: Test individual components in isolation on your development machine

**Framework**: GoogleTest (C++) + Unity (C)

**What We Test**:
- ✅ ButtonInput debouncing state machine
- ✅ LampOutput mode changes and blinking
- ✅ Object Dictionary updates
- ✅ ButtonArray and LampArray management
- ✅ Edge detection logic
- ✅ Time-based state transitions

**Key Strategy**: Mock hardware dependencies
```cpp
// Instead of real HAL:
class MockGPIO : public IGPIOInterface {
    bool readPin(GPIO_Port port, uint16_t pin) override {
        return mockState[pin];  // Controllable in tests
    }
};
```

**Advantages**:
- ⚡ Fast (milliseconds per test)
- 🔄 Run in CI/CD automatically
- 🐛 Easy debugging with IDE
- 💻 No hardware required
- 📊 Code coverage measurement

**Example Test**:
```cpp
TEST(ButtonInputTest, DebounceRejectsShortPulse) {
    ButtonInput button(0, 20);  // 20ms debounce
    button.init();

    // Simulate 5ms press (too short)
    bool changed = button.update(true, 0);    // Press at t=0
    EXPECT_FALSE(changed);
    changed = button.update(false, 5);         // Release at t=5
    EXPECT_FALSE(button.isPressed());          // Should reject
}

TEST(ButtonInputTest, DebounceAcceptsValidPress) {
    ButtonInput button(0, 20);
    button.init();

    // Simulate 25ms press (valid)
    button.update(true, 0);                    // Press at t=0
    bool changed = button.update(true, 25);    // Still pressed at t=25
    EXPECT_TRUE(changed);
    EXPECT_TRUE(button.isPressed());
}
```

### 2. Integration Tests (Simulated)

**Goal**: Test how components work together without physical hardware

**Framework**: Custom test harness with SocketCAN (Linux) or Virtual CAN

**What We Test**:
- ✅ Button press → OD update → TPDO transmission
- ✅ RPDO reception → OD update → Lamp control
- ✅ SDO read/write operations
- ✅ Multi-task synchronization
- ✅ CANopen state machine transitions

**Setup**:
```bash
# Linux with SocketCAN
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

**Example Test**:
```cpp
TEST_F(IntegrationTest, ButtonPressSendsTPDO) {
    // Arrange
    setupVirtualCAN();
    OperatorPanel panel(config);
    panel.init();

    // Act
    simulateButtonPress(0);  // Press button 0
    panel.processButtons(getCurrentTime());

    // Assert
    CANMessage msg = readCANBus();
    EXPECT_EQ(msg.id, 0x180 + NODE_ID);  // TPDO1
    EXPECT_EQ(msg.data[0] & 0x01, 0x01); // Button 0 bit set
}
```

### 3. Hardware-in-Loop (HIL) Tests

**Goal**: Validate on real STM32 hardware with actual I/O

**Tools**:
- Physical test jig with buttons/LEDs
- CAN bus analyzer
- Automated test scripts via debugger

**What We Test**:
- ✅ Real GPIO timing
- ✅ CAN bus electrical characteristics
- ✅ FreeRTOS scheduling under load
- ✅ Power consumption
- ✅ Temperature stability
- ✅ EMC compliance

**Test Jig Design**:
```
┌──────────────────────────────────┐
│  STM32 Device Under Test (DUT)  │
│                                  │
│  Buttons ← GPIO Simulator        │
│  Lamps   → LED + Scope          │
│  CAN     ↔ CAN Analyzer         │
│  Debug   → ST-Link              │
└──────────────────────────────────┘
```

## Test Organization

### Directory Structure

```
tests/
├── unit/                       # Host-based unit tests
│   ├── test_button_input.cpp
│   ├── test_lamp_output.cpp
│   ├── test_operator_panel.cpp
│   ├── test_od.cpp
│   └── mocks/
│       ├── mock_gpio.hpp
│       ├── mock_hal.hpp
│       └── mock_freertos.hpp
├── integration/                # Integration tests
│   ├── test_canopen_pdo.cpp
│   ├── test_canopen_sdo.cpp
│   └── test_task_coordination.cpp
├── hardware/                   # HIL test procedures
│   ├── test_procedures.md
│   ├── test_gpio_timing.py
│   └── test_can_conformance.py
├── CMakeLists.txt             # Test build config
└── README.md                  # How to run tests
```

## Build Configurations

### Unit Tests (Host Build)

```cmake
# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(CANopen-Panel-Tests)

# Use host compiler, not cross-compiler
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

# Include source under test (but not STM32-specific parts)
add_executable(unit_tests
    test_button_input.cpp
    test_lamp_output.cpp
    ../src/application/ButtonInput.cpp
    ../src/application/LampOutput.cpp
    # Note: Exclude STM32 HAL dependencies
)

target_link_libraries(unit_tests gtest_main)
```

### Target Build (STM32)

Existing `CMakeLists.txt` remains for firmware builds.

## Continuous Integration (CI/CD)

### GitHub Actions Workflow

```yaml
# .github/workflows/test.yml
name: Unit Tests

on: [push, pull_request]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++ lcov

      - name: Build Tests
        run: |
          mkdir build-tests && cd build-tests
          cmake ../tests
          make

      - name: Run Tests
        run: |
          cd build-tests
          ctest --output-on-failure

      - name: Coverage Report
        run: |
          cd build-tests
          lcov --capture --directory . --output-file coverage.info
          lcov --remove coverage.info '/usr/*' --output-file coverage.info
          lcov --list coverage.info

  firmware-build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - name: Install ARM Toolchain
        run: |
          sudo apt-get install -y gcc-arm-none-eabi

      - name: Build Firmware
        run: |
          mkdir build && cd build
          cmake -DCMAKE_BUILD_TYPE=Release ..
          make

      - name: Upload Artifacts
        uses: actions/upload-artifact@v3
        with:
          name: firmware
          path: build/*.hex
```

## Hardware Abstraction for Testing

### Interface-Based Design

```cpp
// include/drivers/IGPIOInterface.hpp
class IGPIOInterface {
public:
    virtual ~IGPIOInterface() = default;
    virtual bool readPin(void* port, uint16_t pin) = 0;
    virtual void writePin(void* port, uint16_t pin, bool state) = 0;
};

// Production implementation (STM32)
class STM32GPIO : public IGPIOInterface {
public:
    bool readPin(void* port, uint16_t pin) override {
        return HAL_GPIO_ReadPin((GPIO_TypeDef*)port, pin) == GPIO_PIN_SET;
    }
    void writePin(void* port, uint16_t pin, bool state) override {
        HAL_GPIO_WritePin((GPIO_TypeDef*)port, pin,
                         state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
};

// Test implementation (Mock)
class MockGPIO : public IGPIOInterface {
    std::map<uint16_t, bool> pinStates;
public:
    bool readPin(void* port, uint16_t pin) override {
        return pinStates[pin];
    }
    void writePin(void* port, uint16_t pin, bool state) override {
        pinStates[pin] = state;
    }
    // Test helpers
    void setMockPinState(uint16_t pin, bool state) {
        pinStates[pin] = state;
    }
};
```

### Dependency Injection

```cpp
// Modified OperatorPanel constructor
class OperatorPanel {
public:
    // Production: Use real GPIO
    OperatorPanel(const OperatorPanelConfig& config)
        : OperatorPanel(config, std::make_unique<STM32GPIO>()) {}

    // Testing: Inject mock GPIO
    OperatorPanel(const OperatorPanelConfig& config,
                  std::unique_ptr<IGPIOInterface> gpio)
        : config_(config), gpio_(std::move(gpio)) {}

private:
    std::unique_ptr<IGPIOInterface> gpio_;

    bool readButtonGpio(uint8_t buttonIndex) {
        return gpio_->readPin(config_.buttonPins[buttonIndex].port,
                             config_.buttonPins[buttonIndex].pin);
    }
};
```

## Test Coverage Goals

| Component              | Target Coverage | Current |
|------------------------|-----------------|---------|
| ButtonInput            | 100%            | TBD     |
| LampOutput             | 100%            | TBD     |
| OperatorPanel          | 90%             | TBD     |
| Object Dictionary      | 100%            | TBD     |
| Task coordination      | 80%             | TBD     |
| CANopen integration    | 70% (HIL)       | TBD     |

## Running Tests

### Quick Start

```bash
# Build and run unit tests
cd tests
mkdir build && cd build
cmake ..
make
ctest --verbose

# Run specific test
./test_button_input --gtest_filter=ButtonInputTest.DebounceRejectsShortPulse

# Generate coverage report
make coverage
```

### Pre-Commit Checks

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Run unit tests
cd tests/build
ctest --output-on-failure
if [ $? -ne 0 ]; then
    echo "❌ Unit tests failed. Commit aborted."
    exit 1
fi

# Check code formatting
cd ../..
clang-format --dry-run --Werror src/**/*.cpp
if [ $? -ne 0 ]; then
    echo "❌ Code formatting check failed. Run: make format"
    exit 1
fi

echo "✅ All checks passed"
```

## Test Data & Fixtures

### Test Configuration

```cpp
// tests/fixtures/test_config.hpp
class TestConfig {
public:
    static OperatorPanelConfig createTestConfig() {
        OperatorPanelConfig config = {};
        config.numButtons = 3;
        config.numLamps = 2;
        config.numGpios = 0;
        config.buttonDebounceMs = 20;
        // Mock GPIO pins (values don't matter for mocks)
        config.buttonPins[0] = {nullptr, 0, false};
        config.buttonPins[1] = {nullptr, 1, false};
        config.buttonPins[2] = {nullptr, 2, false};
        config.lampPins[0] = {nullptr, 0, true};
        config.lampPins[1] = {nullptr, 1, true};
        return config;
    }
};
```

## Metrics & Reporting

### What We Measure

1. **Code Coverage**: Lines, branches, functions
2. **Test Execution Time**: Each test should be < 100ms
3. **Memory Usage**: Monitor for leaks in long-running tests
4. **Flakiness**: Track intermittent failures

### CI/CD Dashboard

- ✅ All tests passing: Green build
- 📊 Coverage reports via Codecov/Coveralls
- ⏱️ Performance trends over time
- 📈 Test count growth

## Benefits of This Strategy

### For Development
- 🚀 **Fast feedback**: Unit tests run in seconds
- 🐛 **Easy debugging**: Test failures pinpoint exact issues
- 🔒 **Refactoring confidence**: Tests catch regressions
- 📝 **Documentation**: Tests show usage examples

### For Team
- 👥 **Onboarding**: New developers run tests to verify setup
- 🤝 **Code review**: Tests verify behavior changes
- 📦 **Releases**: Automated testing prevents broken releases

### For Project
- 🎯 **Quality assurance**: Measurable code quality
- 💰 **Cost savings**: Catch bugs before hardware testing
- ⚡ **Development speed**: Iterate without flashing hardware
- 🏆 **Professional credibility**: Industry best practices

## Roadmap

### Phase 1: Foundation (Week 1)
- [ ] Set up GoogleTest framework
- [ ] Create mock interfaces
- [ ] Write first ButtonInput tests
- [ ] Set up CI/CD pipeline

### Phase 2: Core Tests (Week 2)
- [ ] Complete ButtonInput test suite
- [ ] Complete LampOutput test suite
- [ ] Object Dictionary tests
- [ ] Test coverage > 80%

### Phase 3: Integration (Week 3)
- [ ] Integration test framework
- [ ] CANopen PDO/SDO tests
- [ ] Task coordination tests

### Phase 4: Hardware (Ongoing)
- [ ] HIL test procedures
- [ ] Automated hardware tests
- [ ] Performance benchmarking

## Success Criteria

✅ **Definition of Done**:
1. All unit tests pass in CI/CD
2. Code coverage ≥ 80% on critical paths
3. Zero test flakiness (99.9% reliability)
4. Tests run in < 2 minutes total
5. Documentation includes test examples
6. Every PR requires passing tests

## Conclusion

Following this testing strategy ensures:
- **Confidence**: You can verify your work at every level
- **Speed**: Rapid development iteration without hardware
- **Quality**: Catch bugs early, prevent regressions
- **Professionalism**: Industry-standard practices

Remember: **"You need a way to verify your work"** - and now you have multiple ways! 🎯
