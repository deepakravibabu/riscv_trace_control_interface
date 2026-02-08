#pragma once
#include <cstdint>

namespace tci {
    // TraceEncoder control register offsets
    static constexpr uint32_t TR_TE_CONTROL = 0x00;
    
    // TraceFunnel control register offsets
    static constexpr uint32_t TR_FUNNEL_CONTROL = 0x00;
    static constexpr uint32_t TR_FUNNEL_DIS_INPUT = 0x04;
    
    // TraceRamSink control register offsets
    static constexpr uint32_t TR_RAM_CONTROL = 0x00;
    static constexpr uint32_t TR_RAM_WP_LOW = 0x04;
    static constexpr uint32_t TR_RAM_RP_LOW = 0x08;
    static constexpr uint32_t TR_RAM_DATA = 0x0C;
}