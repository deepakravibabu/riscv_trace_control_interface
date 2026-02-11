#pragma once

#include <iostream>
#include <cstdint>
#include <cstddef>

#include "TraceBytesConnect.h"
#include "IMmioDevice.h"
#include "TraceControlRegisters.h"


namespace tci {
    class TraceFunnel : public TraceBytesConnect, public IMmioDevice {
        public:
        TraceFunnel() {
            // std::cout << "[TraceFunnel] constructor called" << std::endl;
        }
        
        ~TraceFunnel() {
            // std::cout << "[TraceFunnel] destructor called" << std::endl;
        }
        
        void connect(TraceBytesConnect* connector) {
            out = connector;
        }
        
        void pushBytes(const std::uint8_t* data, std::size_t length) override {
            const bool active = (trFunnelControl & tci::tr_tf::TR_FUNNEL_ACTIVE) != 0;
            const bool enable = (trFunnelControl & tci::tr_tf::TR_FUNNEL_ENABLE) != 0;
            const bool disInput = (trFunnelDisInput & tci::tr_tf::TR_FUNNEL_DIS_INPUT_MASK) != 0;
            
            if(!active || !enable) {
                std::cout << "[TraceFunnel::pushBytes] Trace funneling is disabled" << std::endl;
                return;
            }

            if (out) {
                if(disInput) {
                    std::cout << "[TraceFunnel::pushBytes] Trace funneling input is disabled" << std::endl;
                    return;
                }
                std::cout << "[TraceFunnel::pushBytes] Pushing bytes to connector" << std::endl;
                out->pushBytes(data, length);
            } else {
                std::cout << "[TraceFunnel::pushBytes] No out set" << std::endl;
            }
        }
        
        // void set_funnelControl(uint32_t control) {
        //     trFunnelControl = control;
        // }
        
        std::uint32_t read32(std::uint32_t offset) override {
            switch (offset) {
                case tci::tr_tf::TR_FUNNEL_CONTROL:
                    return trFunnelControl;
                case tci::tr_tf::TR_FUNNEL_DIS_INPUT:
                    return trFunnelDisInput;
                default:
                    std::cout << "[TraceFunnel::read32] Invalid offset: " << offset << std::endl;
                return 0;
            }
        }
        
        void write32(std::uint32_t offset, std::uint32_t value) override {
            switch (offset) {
            case tci::tr_tf::TR_FUNNEL_CONTROL: {
                const std::uint32_t oldValue = trFunnelControl;

                const bool newActive = (value & tci::tr_tf::TR_FUNNEL_ACTIVE) != 0;
                if(!newActive) {
                    trFunnelControl = 0; // reset all control bits to default values when deactivating
                    std::cout << "[TraceFunnel::write32] TraceFunnel deactivated, internal state reset, control bits cleared" << std::endl;
                    return;
                }

                // keep RO bits(EMPTY) as oldValue
                const std::uint32_t keep_ro = oldValue & tci::tr_tf::TR_FUNNEL_CONTROL_RO_MASK;
                // Normal masked write
                // Take RW bits(ACTIVE, ENABLE) from new value
                const std::uint32_t new_rw  = value & tci::tr_tf::TR_FUNNEL_CONTROL_RW_MASK;
                trFunnelControl = keep_ro | new_rw;

                break;
            }
            case tci::tr_tf::TR_FUNNEL_DIS_INPUT: {
                // take RW bits from new value
                const std::uint32_t new_rw  = value & tci::tr_tf::TR_FUNNEL_DIS_INPUT_RW_MASK;
                trFunnelDisInput = normalize_warl_fields(new_rw);                
                break;
            }
            default:
                std::cout << "[TraceFunnel::write32] Invalid offset: " << offset << std::endl;
            break;
        }
    }
    
    static std::uint32_t normalize_warl_fields(std::uint32_t rw_value) {
        // WARL-lite: clamp fields to legal bitwidth and (optionally) supported subset.
        
        auto clamp_field = [&](std::uint32_t mask, std::uint32_t shift, std::uint32_t max_val) {
            std::uint32_t v = (rw_value & mask) >> shift;
            if (v > max_val) v = max_val; // clamp
            rw_value = (rw_value & ~mask) | ((v << shift) & mask);
        };
        
        // InstMode is 16 bits [15:0] -> legal 0..(2^16-1)
        clamp_field(tci::tr_tf::TR_FUNNEL_DIS_INPUT_MASK,
            0,
            0xFFFFu);
            
        return rw_value;
    }
        
    private:
        TraceBytesConnect* out = nullptr;
        
        std::uint32_t trFunnelControl = 0; // enable = 0 (default)
        std::uint32_t trFunnelDisInput = 0;
    };
}
