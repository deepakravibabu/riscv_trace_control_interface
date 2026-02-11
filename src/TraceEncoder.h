#pragma once

#include <iostream>
#include <vector>
#include <cstdint>

#include "TraceBytesConnect.h"
#include "IMmioDevice.h"
#include "TraceControlRegisters.h"


namespace tci {
    class TraceEncoder : public IMmioDevice{
    public:
    TraceEncoder() {
        // std::cout << "[TraceEncoder] constructor called" << std::endl;
    }

    ~TraceEncoder() {
        // std::cout << "[TraceEncoder] destructor called" << std::endl;
    }
    
    void connect(TraceBytesConnect* connector) {
        out = connector;
    }
    
    void emitTrace(std::uint32_t pc, std::uint32_t opcode) {
        const bool active = (trTeControl & tci::tr_te::TR_TE_ACTIVE) != 0;
        const bool enable = (trTeControl & tci::tr_te::TR_TE_ENABLE) != 0;
        const bool tracing = (trTeControl & tci::tr_te::TR_TE_INST_TRACING) != 0;
        
        if(!active || !enable || !tracing) {
            std::cout << "[TraceEncoder::emitTrace] Trace encoding is inactive or disabled or not tracing, skipping instruction emission" << std::endl;
            return;
        }
    
        if (!out) {
            std::cout << "[TraceEncoder::emitTrace] No out set" << std::endl;
            return;
        }
        
        TraceBytes buffer; // using TraceBytes = std::vector<std::uint8_t>;
        buffer.reserve(8); // reserve space for 2 uint32_t
        append_u32_le(buffer, pc); // pc
        append_u32_le(buffer, opcode); // opcode

        // Status: once we emit something, it is not empty anymore
        trTeControl &= ~tci::tr_te::TR_TE_EMPTY;
        
        std::cout << "[TraceEncoder::emitTrace] buffer filled with pc and opcode" << std::endl;
        
        out->pushBytes(buffer);
    }
    
    std::uint32_t read32(std::uint32_t offset) override {
        switch (offset) {
            case tci::tr_te::TR_TE_CONTROL:
                return trTeControl;
            default:
                std::cout << "[TraceEncoder::read32] Invalid offset: " << std::hex << offset << std::dec << std::endl;
            return 0;
        }
    }
    
    void write32(uint32_t offset, uint32_t value) override {
        switch (offset) {
            case tci::tr_te::TR_TE_CONTROL:{
                const std::uint32_t oldValue = trTeControl;

                // Active bit = 0 (reset); 
                // Active bit = 1 (release reset)
                const bool newActive = (value & tci::tr_te::TR_TE_ACTIVE) != 0;
                if(!newActive) {
                    trTeControl = 0; // reset all control bits to default values when deactivating
                    trTeControl |= tci::tr_te::TR_TE_EMPTY;
                    std::cout << "[TraceEncoder::write32] TraceEncoder deactivated, internal state reset, control bits cleared" << std::endl;
                    return;
                }

                // Normal masked write 
                // Keep RO bits(EMPTY) as oldValue
                const std::uint32_t keep_ro = oldValue & tci::tr_te::TR_TE_CONTROL_RO_MASK;
                
                // Take RW bits(ACTIVE, ENABLE, INST_TRACING, FORMAT) from new value
                std::uint32_t new_rw  = value & tci::tr_te::TR_TE_CONTROL_RW_MASK;

                // multi-bit fields write
                new_rw = normalize_warl_fields(new_rw);

                // register value updated (keep old + update only new)
                trTeControl = keep_ro | new_rw;

                // 3) RW1C behavior: trTeInstStallOrOverflow clears when software writes 1
                if (value & tci::tr_te::TR_TE_INST_STALL_OR_OVERFLOW) {
                    trTeControl &= ~tci::tr_te::TR_TE_INST_STALL_OR_OVERFLOW;
                }

                // 4) If Enable is cleared, it’s reasonable to mark "not tracing" in status
                // if ((trTeControl & tci::tr_te::TR_TE_ENABLE) == 0) {
                //     trTeControl &= ~tci::tr_te::TR_TE_INST_TRACING;
                // }

                // 5) If Active is set (released reset), Empty should usually be 1 until data is emitted
                // if (newActive && !(trTeControl & tci::tr_te::TR_TE_INST_TRACING)) {
                //     trTeControl |= tci::tr_te::TR_TE_EMPTY;
                // }

                break;
            }
            default:
                std::cout << "[TraceEncoder::write32] Invalid offset: " << offset << std::endl;
        }
    }
    
    private:
    TraceBytesConnect* out = nullptr;
    std::uint32_t trTeControl = 0; // enable = 0 (default)
    
    private:
    static void append_u32_le(std::vector<uint8_t>& buffer, uint32_t value) {
        buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
        buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
    }

    static std::uint32_t normalize_warl_fields(std::uint32_t rw_value) {
        // WARL-lite: clamp fields to legal bitwidth and (optionally) supported subset.

        auto clamp_field = [&](std::uint32_t mask, std::uint32_t shift, std::uint32_t max_val) {
            std::uint32_t v = (rw_value & mask) >> shift;
            if (v > max_val) v = max_val; // clamp
            rw_value = (rw_value & ~mask) | ((v << shift) & mask);
        };

        // InstMode is 3 bits [6:4] -> legal 0..7
        clamp_field(tci::tr_te::TR_TE_INST_MODE_MASK,
                    tci::tr_te::TR_TE_INST_MODE_SHIFT,
                    7u);

        // InstSyncMode is 2 bits [17:16] -> legal 0..3
        clamp_field(tci::tr_te::TR_TE_INST_SYNC_MODE_MASK,
                    tci::tr_te::TR_TE_INST_SYNC_MODE_SHIFT,
                    3u);

        // InstSyncMax is 4 bits [23:20] -> legal 0..15
        clamp_field(tci::tr_te::TR_TE_INST_SYNC_MAX_MASK,
                    tci::tr_te::TR_TE_INST_SYNC_MAX_SHIFT,
                    15u);

        // Format is 3 bits [26:24] -> legal 0..7
        clamp_field(tci::tr_te::TR_TE_FORMAT_MASK,
                    tci::tr_te::TR_TE_FORMAT_SHIFT,
                    7u);

        return rw_value;
    }
        
    };
}