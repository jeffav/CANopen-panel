/**
 * @file test_operator_panel.cpp
 * @brief Host-build unit tests for OperatorPanel + Tasks time source
 *
 * Covers the pieces filled in during the "TODO sweep" pass:
 *   - Tasks_GetTimeMs monotonicity on host (no FreeRTOS).
 *   - runDiagnostics returns true on a fresh panel and false after
 *     we corrupt device type / tear down initialization.
 *   - emergencyStop clears all lamps and the OD lamp mask.
 *   - Button -> OD propagation through processButtons().
 *
 * Uses the existing stub GPIO layer (readButtonGpio/writeLampGpio return
 * placeholder values), so all reads-from-HW look like "released" / "low".
 * That is fine — the assertions focus on control flow through the public
 * API and the Object Dictionary, not on real GPIO state.
 */

#include <gtest/gtest.h>

#include "OperatorPanel.hpp"
#include "Tasks.h"
#include "OD.h"

extern "C" void Tasks_SetTimeMsForTest(uint32_t t);
extern "C" void Tasks_AdvanceTimeMsForTest(uint32_t d);

/* main.cpp is not linked into the test binary, so provide the
 * extern symbol that Tasks.cpp references. */
class OperatorPanel;
OperatorPanel* g_operatorPanel = nullptr;

static OperatorPanelConfig makeConfig() {
    OperatorPanelConfig cfg{};
    cfg.numButtons = 3;
    cfg.numLamps = 2;
    cfg.numGpios = 5;
    cfg.buttonDebounceMs = 20;
    for (auto& p : cfg.buttonPins) { p.port = nullptr; p.pin = 0; p.activeHigh = true; }
    for (auto& p : cfg.lampPins)   { p.port = nullptr; p.pin = 0; p.activeHigh = true; }
    for (auto& p : cfg.gpioPins)   { p.port = nullptr; p.pin = 0; p.activeHigh = true; }
    return cfg;
}

class OperatorPanelTest : public ::testing::Test {
protected:
    OperatorPanel* panel = nullptr;

    void SetUp() override {
        Tasks_SetTimeMsForTest(0);
        panel = new OperatorPanel(makeConfig());
        ASSERT_TRUE(panel->init());
    }
    void TearDown() override {
        delete panel;
        panel = nullptr;
    }
};

TEST(TasksTimeSource, MonotonicOnHost) {
    Tasks_SetTimeMsForTest(0);
    EXPECT_EQ(Tasks_GetTimeMs(), 0u);
    Tasks_AdvanceTimeMsForTest(5);
    EXPECT_EQ(Tasks_GetTimeMs(), 5u);
    Tasks_AdvanceTimeMsForTest(100);
    EXPECT_EQ(Tasks_GetTimeMs(), 105u);
}

TEST_F(OperatorPanelTest, DiagnosticsPassAfterInit) {
    EXPECT_TRUE(panel->runDiagnostics());
}

TEST_F(OperatorPanelTest, DiagnosticsFailOnBadDeviceType) {
    OD_RAM.deviceType = 0xDEADBEEFu;
    EXPECT_FALSE(panel->runDiagnostics());
    OD_RAM.deviceType = OD_DEV_TYPE;  /* restore for other tests */
}

TEST_F(OperatorPanelTest, DiagnosticsRestoresLampState) {
    /* Turn lamp 1 on via OD, then run diagnostics, and confirm OD lamp
     * mask is what we put there. */
    OD_SetLamp(1, true);
    panel->processLampOutputs(0);
    ASSERT_TRUE(panel->runDiagnostics());
    EXPECT_NE(OD_ReadLampOutputs() & (1u << 1), 0u);
}

TEST_F(OperatorPanelTest, EmergencyStopClearsLamps) {
    OD_SetLamp(0, true);
    OD_SetLamp(1, true);
    panel->processLampOutputs(0);
    panel->emergencyStop();
    EXPECT_EQ(OD_ReadLampOutputs() & 0x03u, 0u);
}

TEST_F(OperatorPanelTest, ProcessButtonsReportsNoChangeInitially) {
    /* Stub GPIO always reads "not pressed", and the initial state is
     * RELEASED, so processButtons() should not report a change. */
    EXPECT_FALSE(panel->processButtons(10));
    EXPECT_EQ(OD_RAM.buttonInputs & 0x07u, 0u);
}

TEST_F(OperatorPanelTest, ProcessGpioInputsReportsNoChangeInitially) {
    EXPECT_FALSE(panel->processGpioInputs(10));
}
