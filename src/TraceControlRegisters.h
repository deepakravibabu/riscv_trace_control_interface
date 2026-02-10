#pragma once
#include <cstdint>

namespace tci {

    // TraceEncoder control register offsets
    namespace tr_te {
        // trTeControl (trBaseTe+0x000)
        static constexpr uint32_t TR_TE_CONTROL = 0x000;
        // Single-bit fields
        static constexpr uint32_t TR_TE_ACTIVE                  = 0x1u << 0;        // trTeActive -> TR_TE_CONTROL[0]
        static constexpr uint32_t TR_TE_ENABLE                  = 0x1u << 1;        // trTeEnable -> TR_TE_CONTROL[1]
        static constexpr uint32_t TR_TE_INST_TRACING            = 0x1u << 2;        // trTeInstTracing -> TR_TE_CONTROL[2]
        static constexpr uint32_t TR_TE_EMPTY                   = 0x1u << 3;        // trTeEmpty -> TR_TE_CONTROL[3]
        static constexpr uint32_t TR_TE_CONTEXT                 = 0x1u << 9;        // trTeContext -> TR_TE_CONTROL[9]
        static constexpr uint32_t TR_TE_INST_TRIG_ENABLE        = 0x1u << 11;       // trTeInstTrigEnable -> TR_TE_CONTROL[11]
        static constexpr uint32_t TR_TE_INST_STALL_OR_OVERFLOW  = 0x1u << 12;       // trTeInstStallOrOverflow -> TR_TE_CONTROL[12]
        static constexpr uint32_t TR_TE_INST_STALL_ENA          = 0x1u << 13;       // trTeInstStallEna -> TR_TE_CONTROL[13]
        static constexpr uint32_t TR_TE_INHIBIT_SRC             = 0x1u << 15;       // trTeInhibitSrc -> TR_TE_CONTROL[15] = 0        
        // Multi-bit fields SHIFT + MASK
        static constexpr uint32_t TR_TE_INST_MODE_SHIFT         = 4;
        static constexpr uint32_t TR_TE_INST_MODE_MASK          = 0x7u << TR_TE_INST_MODE_SHIFT;    // trTeInstMode -> TR_TE_CONTROL[6:4]
        static constexpr uint32_t TR_TE_INST_SYNC_MODE_SHIFT    = 16;
        static constexpr uint32_t TR_TE_INST_SYNC_MODE_MASK     = 0x3u << TR_TE_INST_SYNC_MODE_SHIFT;        // trTeInstSyncMode -> TR_TE_CONTROL[17:16]
        static constexpr uint32_t TR_TE_INST_SYNC_MAX_SHIFT     = 20;
        static constexpr uint32_t TR_TE_INST_SYNC_MAX_MASK      = 0xFu << TR_TE_INST_SYNC_MAX_SHIFT;       // trTeInstSyncMax -> TR_TE_CONTROL[23:20]
        static constexpr uint32_t TR_TE_FORMAT_SHIFT            = 24;
        static constexpr uint32_t TR_TE_FORMAT_MASK             = 0x7u << TR_TE_FORMAT_SHIFT;       // trTeFormat -> TR_TE_CONTROL[26:24]
        // Masks for write behavior 
        // (only bits in CONTROL_RW_MASK can be written, bits in CONTROL_RO_MASK are read-only status bits that may be updated by the TraceEncoder's internal logic)
        static constexpr uint32_t TR_TE_CONTROL_RW_MASK =
            TR_TE_ACTIVE | TR_TE_ENABLE | TR_TE_INST_TRACING | TR_TE_CONTEXT | TR_TE_INST_TRIG_ENABLE | TR_TE_INST_STALL_ENA | TR_TE_INHIBIT_SRC | TR_TE_INST_STALL_OR_OVERFLOW |
            TR_TE_INST_MODE_MASK | TR_TE_INST_SYNC_MODE_MASK | TR_TE_INST_SYNC_MAX_MASK | TR_TE_FORMAT_MASK;
        static constexpr uint32_t TR_TE_CONTROL_RO_MASK =
            TR_TE_EMPTY ; // if you model them as RO status
    }
    
    // TraceFunnel control register offsets
    namespace tr_tf {
        // trFunnelControl (trBaseFunnel+0x000)
        static constexpr uint32_t TR_FUNNEL_CONTROL             = 0x000;
        // Control bit definitions for TR_FUNNEL_CONTROL
        static constexpr uint32_t TR_FUNNEL_ACTIVE              = 0x1u << 0; // trFunnelActive -> TR_FUNNEL_CONTROL[0]
        static constexpr uint32_t TR_FUNNEL_ENABLE              = 0x1u << 1; // trFunnelEnable -> TR_FUNNEL_CONTROL[1]
        static constexpr uint32_t TR_FUNNEL_EMPTY               = 0x1u << 3; // trFunnelEmpty -> TR_FUNNEL_CONTROL[3]
        // Masks for read/write behavior
        static constexpr uint32_t TR_FUNNEL_CONTROL_RW_MASK =
            TR_FUNNEL_ACTIVE | TR_FUNNEL_ENABLE;
        static constexpr uint32_t TR_FUNNEL_CONTROL_RO_MASK =
            TR_FUNNEL_EMPTY; // if you model them as RO status

        // trFunnelDisInput (trBaseFunnel+0x008)
        static constexpr uint32_t TR_FUNNEL_DIS_INPUT           = 0x008; // disable input to the funnel, used to stop accepting new trace data while allowing existing data to be read out from the funnel and sink
        // Control bit definitions for TR_FUNNEL_DIS_INPUT
        static constexpr uint32_t TR_FUNNEL_DIS_INPUT_MASK       = 0x0000FFFFu; // trFunnelDisInput -> TR_FUNNEL_DIS_INPUT[15:0]
        // Masks for read/write behavior
        static constexpr uint32_t TR_FUNNEL_DIS_INPUT_RW_MASK =
            TR_FUNNEL_DIS_INPUT_MASK;
    }
    
    // TraceRamSink control register offsets
    namespace tr_ram {
        // trRamControl (trBaseRamSink+0x000)
        static constexpr uint32_t TR_RAM_CONTROL = 0x000;
        // Control bit definitions for TR_RAM_CONTROL
        static constexpr uint32_t TR_RAM_ACTIVE                 = 0x1u << 0; // trRamActive -> TR_RAM_CONTROL[0]
        static constexpr uint32_t TR_RAM_ENABLE                 = 0x1u << 1; // trRamEnable -> TR_RAM_CONTROL[1]
        static constexpr uint32_t TR_RAM_EMPTY                  = 0x1u << 3; // trRamEmpty -> TR_RAM_CONTROL[3]
        static constexpr uint32_t TR_RAM_MODE                   = 0x1u << 4; // trRamMode -> TR_RAM_CONTROL[4] = 0 for SRAM, 1 for SMEM
        static constexpr uint32_t TR_RAM_STOP_ON_WRAP           = 0x1u << 8; // trRamStopOnWrap -> TR_RAM_CONTROL[8]
        static constexpr uint32_t TR_RAM_MEM_FORMAT_SHIFT       = 9;
        static constexpr uint32_t TR_RAM_MEM_FORMAT_MASK        = 0x3u << TR_RAM_MEM_FORMAT_SHIFT; // trRamMemFormat -> TR_RAM_CONTROL[10:9]
        static constexpr uint32_t TR_RAM_ASYNC_FREQ_SHIFT       = 12;
        static constexpr uint32_t TR_RAM_ASYNC_FREQ_MASK        = 0x7u << TR_RAM_ASYNC_FREQ_SHIFT; // trRamAsyncFreq -> TR_RAM_CONTROL[14:12]
        static constexpr uint32_t TR_RAM_CONTROL_RW_MASK =
            TR_RAM_ACTIVE | TR_RAM_ENABLE | TR_RAM_MODE | TR_RAM_STOP_ON_WRAP | TR_RAM_MEM_FORMAT_MASK | TR_RAM_ASYNC_FREQ_MASK;
        static constexpr uint32_t TR_RAM_CONTROL_RO_MASK =
            TR_RAM_EMPTY ; // if you model them as RO status

        // trRamWPLow (trBaseRamSink+0x020)
        static constexpr uint32_t TR_RAM_WP_LOW = 0x020;
        // Control bit definitions for TR_RAM_WP_LOW
        static constexpr uint32_t TR_RAM_WRAP                   = 0x1u << 0; // trRamWrap -> TR_RAM_WP_LOW[0]
        static constexpr uint32_t TR_RAM_WP_LOW_MASK            = 0xFFFFFFFCu; // trRamWPLow -> TR_RAM_WP_LOW[31:2]
        // Masks for read/write behavior
        static constexpr uint32_t TR_RAM_WP_LOW_RW_MASK =
            TR_RAM_WRAP | TR_RAM_WP_LOW_MASK;
        
        // trRamRPLow (trBaseRamSink+0x028)
        static constexpr uint32_t TR_RAM_RP_LOW = 0x028;
        // Control bit definitions for TR_RAM_RP_LOW
        static constexpr uint32_t TR_RAM_RP_LOW_MASK            = 0xFFFFFFFCu; // trRamRPLow -> TR_RAM_RP_LOW[31:2]
        // Masks for read/write behavior
        static constexpr uint32_t TR_RAM_RP_LOW_RW_MASK =
            TR_RAM_RP_LOW_MASK;

        // trRamData (trBaseRamSink+0x040)
        static constexpr uint32_t TR_RAM_DATA = 0x040;
        // Control bit definitions for TR_RAM_DATA
        static constexpr uint32_t TR_RAM_DATA_SHIFT             = 0;
        static constexpr uint32_t TR_RAM_DATA_MASK              = 0xFFFFFFFFu << TR_RAM_DATA_SHIFT; // trRamData -> TR_RAM_DATA[31:0]
        // Masks for read/write behavior
        // TR_RAM_DATA writes are ignored
    }
}
