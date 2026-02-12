
/* 
    Bash Terminal
        Set GoogleTests OFF and Build 'tci_demo' exec
        cmake -S . -B build -DTCI_BUILD_GTESTS=OFF
        cmake --build build --target tci_demo
        ./build/Debug/tci_demo

        Set GoogleTests ON - Built both executables
        cmake -S . -B build -DTCI_BUILD_GTESTS=ON
        cmake --build build --target tci_demo
        cmake --build build --target tci_gtests
        ./build/Debug/tci_gtests
*/

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

#include "TraceSystem.h"
#include "TraceControllerInterface.h"
#include "ProbeHwAccess.h"
#include "TraceControlRegisters.h"

using namespace tci;

// A fixture that creates a fresh TraceSystem + Probe + TCI per test
class TciFixture : public ::testing::Test {
protected:
    TraceSystem trSystem{1024};

    std::vector<ProbeHwAccess::ComponentRegion> regions = {
        {TraceSystem::TR_TE_BASE,       0x1000, "TraceEncoder"},
        {TraceSystem::TR_FUNNEL_BASE,   0x1000, "TraceFunnel"},
        {TraceSystem::TR_RAM_SINK_BASE, 0x1000, "TraceRamSink"}
    };

    ProbeHwAccess probe{trSystem.mmioBus, regions};

    TraceControllerInterface tci{
        probe,
        TraceSystem::TR_TE_BASE,
        TraceSystem::TR_FUNNEL_BASE,
        TraceSystem::TR_RAM_SINK_BASE
    };
};

TEST_F(TciFixture, ConfigureSetsExpectedControlBits) {
    tci.configure();

    // TRS control bits
    const uint32_t trs = probe.ReadMemory(TraceSystem::TR_RAM_SINK_BASE + tr_ram::TR_RAM_CONTROL);
    EXPECT_TRUE((trs & tr_ram::TR_RAM_ACTIVE) != 0);
    EXPECT_TRUE((trs & tr_ram::TR_RAM_ENABLE) != 0);
    
    // TF control bits
    const uint32_t tf = probe.ReadMemory(TraceSystem::TR_FUNNEL_BASE + tr_tf::TR_FUNNEL_CONTROL);
    EXPECT_TRUE((tf & tr_tf::TR_FUNNEL_ACTIVE) != 0);
    EXPECT_TRUE((tf & tr_tf::TR_FUNNEL_ENABLE) != 0);
    const uint32_t tfDisInput = probe.ReadMemory(TraceSystem::TR_FUNNEL_BASE + tr_tf::TR_FUNNEL_DIS_INPUT);
    EXPECT_TRUE((tfDisInput & tr_tf::TR_FUNNEL_DIS_INPUT_MASK) == 0); // input enabled
    
    // TE control bits
    const uint32_t te = probe.ReadMemory(TraceSystem::TR_TE_BASE + tr_te::TR_TE_CONTROL);
    EXPECT_TRUE((te & tr_te::TR_TE_ACTIVE) != 0);
    EXPECT_TRUE((te & tr_te::TR_TE_INST_TRACING) != 0);
    EXPECT_TRUE(((te & tr_te::TR_TE_FORMAT_MASK) >> tr_te::TR_TE_FORMAT_SHIFT) == 0x5u); // expected format value
    // assert(bitFieldGet(readBackValue, tci::tr_te::TR_TE_FORMAT_MASK, tci::tr_te::TR_TE_FORMAT_SHIFT) == 0x5u);
}

TEST_F(TciFixture, StartIsReadModifyWriteAndDoesNotClobberConfiguration) {
    tci.configure();

    const uint32_t before = probe.ReadMemory(TraceSystem::TR_TE_BASE + tr_te::TR_TE_CONTROL);

    tci.start();

    const uint32_t after = probe.ReadMemory(TraceSystem::TR_TE_BASE + tr_te::TR_TE_CONTROL);

    // Enable bit must be set
    EXPECT_TRUE((after & tr_te::TR_TE_ENABLE) != 0);

    // All other bits should remain at least preserved from before
    // (Enable is allowed to change from 0->1; everything else should match)
    const uint32_t mask_except_enable = ~tr_te::TR_TE_ENABLE;
    EXPECT_EQ((before & mask_except_enable), (after & mask_except_enable));
}

TEST_F(TciFixture, StopClearsEnableBits) {
    tci.configure();
    tci.start();
    tci.stop();

    const uint32_t te = probe.ReadMemory(TraceSystem::TR_TE_BASE + tr_te::TR_TE_CONTROL);
    EXPECT_TRUE((te & tr_te::TR_TE_ENABLE) == 0);

    const uint32_t tf = probe.ReadMemory(TraceSystem::TR_FUNNEL_BASE + tr_tf::TR_FUNNEL_CONTROL);
    EXPECT_TRUE((tf & tr_tf::TR_FUNNEL_ENABLE) == 0);

    const uint32_t trs = probe.ReadMemory(TraceSystem::TR_RAM_SINK_BASE + tr_ram::TR_RAM_CONTROL);
    EXPECT_TRUE((trs & tr_ram::TR_RAM_ENABLE) == 0);
}

TEST_F(TciFixture, EmitAndFetchReturnsExpectedWords) {
    tci.configure();
    tci.start();

    // Emit 2 trace packets: each emits pc then opcode (8 bytes = 2 words)
    trSystem.emitTrace(0x3000, 0xDEADBEEF);
    trSystem.emitTrace(0x3004, 0xCAFEBABE);

    // Stop producer (optional: depending on your design you can fetch while enabled too)
    tci.stop();

    auto out = tci.fetch(4);

    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 0x3000u);
    EXPECT_EQ(out[1], 0xDEADBEEFu);
    EXPECT_EQ(out[2], 0x3004u);
    EXPECT_EQ(out[3], 0xCAFEBABEu);
}

TEST_F(TciFixture, FetchWpRpMatchEmptyFlagSet) {
    tci.configure();
    tci.start();

    trSystem.emitTrace(0x3000, 0x11111111);

    tci.stop();

    // Before fetch, sink should not be empty (ideally).
    auto ctrl_before = probe.ReadMemory(TraceSystem::TR_RAM_SINK_BASE + tci::tr_ram::TR_RAM_CONTROL);
    EXPECT_EQ((ctrl_before & tci::tr_ram::TR_RAM_EMPTY) != 0, false);

    auto out = tci.fetch(100);

    EXPECT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 0x3000u);
    EXPECT_EQ(out[1], 0x11111111u);

    // After fetch, sink should be empty OR pointers should match
    auto wp = probe.ReadMemory(TraceSystem::TR_RAM_SINK_BASE + tci::tr_ram::TR_RAM_WP_LOW);
    auto rp = probe.ReadMemory(TraceSystem::TR_RAM_SINK_BASE + tci::tr_ram::TR_RAM_RP_LOW);
    auto ctrl_after = probe.ReadMemory(TraceSystem::TR_RAM_SINK_BASE + tci::tr_ram::TR_RAM_CONTROL);

    // One of these must indicate empty (depending on how you model it)
    EXPECT_TRUE((wp == rp) && ((ctrl_after & tci::tr_ram::TR_RAM_EMPTY) != 0));
}
