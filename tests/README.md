# CANopen Panel Tests

This directory contains the test suite for the CANopen Operator Panel project.

## Test Organization

```
tests/
├── unit/                  # Unit tests (host-based, fast)
│   ├── test_button_input.cpp
│   ├── test_lamp_output.cpp
│   └── test_od.cpp
├── integration/           # Integration tests (future)
├── hardware/              # HIL test procedures (future)
├── mocks/                 # Mock implementations
│   └── mock_gpio.hpp
├── fixtures/              # Test fixtures and configurations
└── CMakeLists.txt         # Build configuration
```

## Running Tests

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install cmake g++ lcov

# macOS
brew install cmake lcov

# Windows (MinGW or WSL recommended)
```

### Quick Start

```bash
# From the tests directory
mkdir build && cd build
cmake ..
make
ctest --verbose
```

### Individual Test Execution

```bash
# Run specific test suite
./test_button_input

# Run specific test case
./test_button_input --gtest_filter=ButtonInputTest.DebounceRejectsShortPulse

# List all tests
./test_button_input --gtest_list_tests
```

### Build Configurations

```bash
# Debug build (with coverage)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Code Coverage

### Generate Coverage Report

```bash
# Build with coverage enabled
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Generate coverage report
make coverage

# View HTML report
open coverage_html/index.html  # macOS
xdg-open coverage_html/index.html  # Linux
```

### Current Coverage Goals

| Component      | Target | Current |
|----------------|--------|---------|
| ButtonInput    | 100%   | TBD     |
| LampOutput     | 100%   | TBD     |
| OD (C code)    | 100%   | TBD     |
| OperatorPanel  | 90%    | TBD     |

## Test Examples

### Unit Test Example

```cpp
TEST(ButtonInputTest, DebounceRejectsShortPulse) {
    ButtonInput button(0, 20);  // 20ms debounce
    button.init();

    MockTime mockTime;
    mockTime.setTime(0);
    button.update(true, mockTime.getCurrentTimeMs());

    mockTime.setTime(5);  // Only 5ms - too short
    button.update(false, mockTime.getCurrentTimeMs());

    EXPECT_FALSE(button.isPressed());  // Should be rejected
}
```

### Using Mocks

```cpp
TEST(OperatorPanelTest, ButtonPressUpdatesOD) {
    MockGPIO mockGpio;
    OperatorPanelConfig config = TestConfig::createTestConfig();

    // Inject mock GPIO
    OperatorPanel panel(config, std::make_unique<MockGPIO>());

    // Simulate button press
    mockGpio.setMockPinState(0, true);

    // Verify OD updated
    bool changed = panel.processButtons(getCurrentTime());
    EXPECT_TRUE(changed);
}
```

## Continuous Integration

Tests run automatically on every push via GitHub Actions.

### CI Workflow

```yaml
- Build tests
- Run all tests
- Generate coverage report
- Upload coverage to Codecov
```

### Pre-Commit Hook

Install the pre-commit hook to run tests before every commit:

```bash
# From project root
cp tests/pre-commit-hook.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## Writing New Tests

### 1. Create Test File

```cpp
// tests/unit/test_my_component.cpp
#include <gtest/gtest.h>
#include "MyComponent.hpp"

TEST(MyComponentTest, BasicFunctionality) {
    MyComponent component;
    EXPECT_TRUE(component.isValid());
}
```

### 2. Add to CMakeLists.txt

```cmake
add_executable(test_my_component
    unit/test_my_component.cpp
    ${CMAKE_SOURCE_DIR}/../src/MyComponent.cpp
)
target_link_libraries(test_my_component gtest_main)
gtest_discover_tests(test_my_component)
```

### 3. Run and Verify

```bash
make test_my_component
./test_my_component --gtest_color=yes
```

## Test Naming Conventions

### Test Suite Names
- `ClassName` + `Test` → `ButtonInputTest`
- `FunctionalArea` + `Test` → `DebounceTest`

### Test Case Names
- Use descriptive names: `DebounceRejectsShortPulse`
- Follow pattern: `MethodName_Condition_ExpectedBehavior`
- Examples:
  - `TurnOn_WhenOff_TurnsLampOn`
  - `Update_WithValidPress_ReturnsTrue`

## GoogleTest Features Used

### Assertions
- `EXPECT_EQ(a, b)` - Expect equality (continues on failure)
- `ASSERT_EQ(a, b)` - Assert equality (stops on failure)
- `EXPECT_TRUE(condition)` - Expect true
- `EXPECT_FALSE(condition)` - Expect false

### Test Fixtures
```cpp
class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};
```

### Parameterized Tests
```cpp
class MyParamTest : public ::testing::TestWithParam<int> {};

TEST_P(MyParamTest, WorksWithMultipleInputs) {
    int param = GetParam();
    EXPECT_GE(param, 0);
}

INSTANTIATE_TEST_SUITE_P(
    Values,
    MyParamTest,
    ::testing::Values(1, 2, 3, 4, 5)
);
```

## Debugging Tests

### Using GDB

```bash
# Run test under debugger
gdb ./test_button_input
(gdb) run --gtest_filter=ButtonInputTest.DebounceRejectsShortPulse
(gdb) break ButtonInput::update
```

### VS Code Debug Configuration

```json
{
    "type": "cppdbg",
    "request": "launch",
    "name": "Debug Unit Test",
    "program": "${workspaceFolder}/tests/build/test_button_input",
    "args": ["--gtest_filter=*DebounceRejectsShortPulse"],
    "cwd": "${workspaceFolder}/tests/build"
}
```

## Performance Testing

### Benchmarking Individual Tests

```bash
# Run with time measurement
time ./test_button_input

# Individual test timing
./test_button_input --gtest_filter=*Performance* --gtest_repeat=1000
```

### Performance Goals

- Each unit test: < 1ms
- Full test suite: < 5 seconds
- Coverage generation: < 30 seconds

## Troubleshooting

### Common Issues

**Issue**: GoogleTest not found
```bash
# Solution: Let CMake fetch it automatically
rm -rf build
mkdir build && cd build
cmake ..
```

**Issue**: Coverage not generated
```bash
# Ensure Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
# Install lcov
sudo apt-get install lcov
```

**Issue**: Tests fail with linker errors
```bash
# Check that source files are listed in CMakeLists.txt
# Verify include paths are correct
```

## Best Practices

1. ✅ **One assertion per concept** - Test one thing at a time
2. ✅ **Descriptive names** - Test names should explain what they verify
3. ✅ **Fast tests** - Unit tests should run in milliseconds
4. ✅ **Independent tests** - No dependencies between tests
5. ✅ **Readable tests** - Tests are documentation
6. ✅ **Mock external dependencies** - Test in isolation
7. ✅ **Test edge cases** - Zero, max, negative values
8. ✅ **Test error conditions** - Invalid inputs, failures

## Resources

- [GoogleTest Documentation](https://google.github.io/googletest/)
- [GoogleTest Primer](https://google.github.io/googletest/primer.html)
- [C++ Testing Best Practices](https://google.github.io/googletest/gmock_for_dummies.html)
- [Test-Driven Development](https://martinfowler.com/bliki/TestDrivenDevelopment.html)

## Contributing

When adding new functionality:

1. Write tests first (TDD approach preferred)
2. Ensure all tests pass: `make && ctest`
3. Verify coverage: `make coverage`
4. Document test cases in code
5. Update this README if needed
