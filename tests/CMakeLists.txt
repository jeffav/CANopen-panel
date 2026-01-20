cmake_minimum_required(VERSION 3.22)

# Project for tests (host build, not cross-compilation)
project(CANopen-Panel-Tests LANGUAGES CXX C)

# Use host compiler, not ARM cross-compiler
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Enable testing
enable_testing()

# Compiler flags for tests
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

# Add coverage flags for Debug builds
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
endif()

# GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../include/application
    ${CMAKE_SOURCE_DIR}/../include/canopen
    ${CMAKE_SOURCE_DIR}/mocks
    ${CMAKE_SOURCE_DIR}/fixtures
)

# Source files from main project (platform-independent)
set(APP_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/application/ButtonInput.cpp
    ${CMAKE_SOURCE_DIR}/../src/application/LampOutput.cpp
    ${CMAKE_SOURCE_DIR}/../src/canopen/OD.c
)

# Unit test executables
add_executable(test_button_input
    unit/test_button_input.cpp
    ${CMAKE_SOURCE_DIR}/../src/application/ButtonInput.cpp
)
target_link_libraries(test_button_input gtest_main)

add_executable(test_lamp_output
    unit/test_lamp_output.cpp
    ${CMAKE_SOURCE_DIR}/../src/application/LampOutput.cpp
)
target_link_libraries(test_lamp_output gtest_main)

add_executable(test_od
    unit/test_od.cpp
    ${CMAKE_SOURCE_DIR}/../src/canopen/OD.c
)
target_link_libraries(test_od gtest_main)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(test_button_input)
gtest_discover_tests(test_lamp_output)
gtest_discover_tests(test_od)

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_button_input test_lamp_output test_od
    COMMENT "Running all unit tests"
)

# Coverage target (requires lcov)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_custom_target(coverage
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' '*/googletest/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        DEPENDS test_button_input test_lamp_output test_od
        COMMENT "Generating code coverage report"
    )
endif()

# Print configuration
message(STATUS "")
message(STATUS "CANopen Panel - Test Suite Configuration")
message(STATUS "=========================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "GoogleTest: Fetched from GitHub")
message(STATUS "")
