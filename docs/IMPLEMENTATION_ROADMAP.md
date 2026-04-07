# Implementation Roadmap

This document tracks what remains after the "TODO sweep" pass that fleshed out
host-testable logic. Everything below requires either an STM32CubeMX
regeneration, a connected STM32F413RGT6 board, or both, and so could not be
completed in a host-only environment.

## Done in the sweep pass
- Tasks.cpp: real task bodies (FreeRTOS-guarded), `Tasks_GetTimeMs`,
  `Tasks_Init` with `xTaskCreate`, test hooks on host build.
- OperatorPanel.cpp: `runDiagnostics()` now exercises lamps through OD and
  verifies device-type integrity.
- CMakeLists.txt: MCU target corrected from `STM32F407xx` to `STM32F413xx` to
  match [config/MicroXplorer.ioc](../config/).
- config/FreeRTOSConfig.h: minimal config for 1 kHz tick, 16 KB heap_4.
- tests/unit/test_operator_panel.cpp: host tests for OperatorPanel +
  `Tasks_GetTimeMs` monotonicity.

## Blocked on STM32CubeMX regeneration
Regenerate the project from the `.ioc` with code-generation target = Makefile
or CMake. Copy the following outputs into the repo:

1. **Linker script** → `config/STM32F413RGTx_FLASH.ld`
   - Referenced by [CMakeLists.txt:46](../CMakeLists.txt#L46); build will fail
     at link time until present.
2. **Startup assembly** → `src/hal/startup_stm32f413xx.s`
3. **System init** → `src/hal/system_stm32f4xx.c`
4. **HAL config** → `include/hal/stm32f4xx_hal_conf.h`
5. **Interrupt handlers** → `src/hal/stm32f4xx_it.c` (must route
   `SVC_Handler`, `PendSV_Handler`, `SysTick_Handler` to FreeRTOS per
   [config/FreeRTOSConfig.h](../config/FreeRTOSConfig.h))
6. **MSP init** → `src/hal/stm32f4xx_hal_msp.c`
7. **CAN1 init** (`MX_CAN1_Init`) — called from
   [src/main.cpp:86](../src/main.cpp#L86)
8. **GPIO init** (`MX_GPIO_Init`) — called from
   [src/main.cpp:91](../src/main.cpp#L91); actual pin assignments live in the
   `.ioc`.

Add the HAL sources + CMSIS device headers either as a git submodule under
`lib/STM32CubeF4/` or as vendored files under `lib/stm32/`. Update
`CMakeLists.txt` to include the HAL include paths and the startup/system
sources.

## Blocked on hardware wiring (requires the `.ioc` pin map)
Fill in the four leaf functions in
[src/application/OperatorPanel.cpp](../src/application/OperatorPanel.cpp):

- `readButtonGpio` ([line 123](../src/application/OperatorPanel.cpp#L123))
- `readGpioInput` ([line 146](../src/application/OperatorPanel.cpp#L146))
- `writeLampGpio` ([line 159](../src/application/OperatorPanel.cpp#L159))
- `initGpio` ([line 214](../src/application/OperatorPanel.cpp#L214)) — pin
  configs can be read from `config_.buttonPins[]`, `config_.lampPins[]`,
  `config_.gpioPins[]`, but the instance that populates those configs (in
  `main.cpp` around [line 39](../src/main.cpp#L39)) needs the real port/pin
  constants from the `.ioc`.

## Blocked on CanOpenSTM32 driver integration
Once HAL is present:

1. Uncomment `CANopen_Init` body in
   [src/main.cpp:103](../src/main.cpp#L103) and wire the CanOpenSTM32 driver
   (`lib/CanOpenSTM32/`).
2. Uncomment the `HAL_CAN_RxFifo0MsgPendingCallback` forwarder in
   [src/main.cpp:202](../src/main.cpp#L202).
3. Build with `-DUSE_CANOPEN` so the `CO_process` / `CO_TPDOsendRequest`
   calls in [src/application/Tasks.cpp](../src/application/Tasks.cpp) become
   active.
4. Uncomment `vTaskStartScheduler()` in
   [src/main.cpp:185](../src/main.cpp#L185). **Do not do this before steps
   1-3** — the scheduler will tail-spin on an uninitialized CO object.

## Verification plan once the above lands
1. `cmake -B build && cmake --build build` — produces `.elf`/`.hex`/`.bin`,
   `--print-memory-usage` should succeed.
2. Flash to F413 dev board. Heartbeat LED on lamp 0 should blink via the
   `Task_LampControl` path within ~1 s of reset.
3. With `candump can0` on a connected host:
   - Verify 0x70A (heartbeat) appears every `producerHeartbeatTime` ms.
   - Press a button; verify TPDO1 (0x18A) carries the updated button mask.
   - Send an RPDO1 (0x20A) with lamp bits set; verify lamp GPIO tracks.
4. Run `runDiagnostics()` from a debug shell or a one-shot boot diagnostics
   hook; expect `true` on good hardware.
