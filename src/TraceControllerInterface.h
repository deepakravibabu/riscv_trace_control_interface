#pragma once
#include <cstdint>
#include <vector>
#include <cassert>

#include "MmioBus.h"
#include "TraceControlRegisters.h"


namespace tci {
    class TraceControllerInterface {
    public:
            TraceControllerInterface(MmioBus& bus, uint32_t teBase, uint32_t funnelBase, uint32_t ramSinkBase)
                : mmioBus(bus), trTeBase(teBase), trFunnelBase(funnelBase), trRamSinkBase(ramSinkBase) {}

                
                public:
                static void expectBits(uint32_t got, uint32_t mask, bool should_set){
                    if(should_set) assert((got & mask) == mask);
                    else assert((got & mask) == 0);
    }
    
    static inline uint32_t bitFieldGet(uint32_t reg, uint32_t mask, uint32_t shift) {
        return (reg & mask) >> shift;
    }
    static inline uint32_t bitFieldSet(uint32_t reg, uint32_t mask, uint32_t shift, uint32_t v) {
        reg &= ~mask;
        reg |= (v << shift) & mask;
        return reg;
    }
    
    
    void configure() {
        // Enable TraceEncoder
        // mmioBus.write32(trTeBase + tci::tr_te::TR_TE_CONTROL, 1);
        // enable trTeActive and trTeEnable, set trTeFormat to 0 (default)
        uint32_t teControlValue = tci::tr_te::TR_TE_ACTIVE | tci::tr_te::TR_TE_ENABLE | tci::tr_te::TR_TE_INST_TRACING
        | (0x5u << tci::tr_te::TR_TE_FORMAT_SHIFT) & tci::tr_te::TR_TE_FORMAT_MASK;
        mmioBus.write32(trTeBase + tci::tr_te::TR_TE_CONTROL, teControlValue);
        
        // Assertions to check the write
        uint32_t readBackValue = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL);
        expectBits(readBackValue, tci::tr_te::TR_TE_ACTIVE, true);
        expectBits(readBackValue, tci::tr_te::TR_TE_ENABLE, true);
        // Assertion
        uint32_t readBackFormat = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL);
        assert(bitFieldGet(readBackFormat, tci::tr_te::TR_TE_FORMAT_MASK, tci::tr_te::TR_TE_FORMAT_SHIFT) == 0x5u);
        
        
        // Enable TraceFunnel
        mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL, 1);
        mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_DIS_INPUT, 1); // enable funnel input
        // Enable TraceRamSink
        mmioBus.write32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL, 1);
    }
    
    void start() {
        // For this simple implementation, configure() already starts the tracing
        // In a more complex implementation, you might have separate control bits for starting/stopping
    }
    
    void stop() {
        // Disable Producer first to stop new data from being generated
        // Disable TraceEncoder
        mmioBus.write32(trTeBase + tci::tr_te::TR_TE_CONTROL, 0);
        // Disable TraceFunnel
        mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL, 0);
        // Disable TraceRamSink
        mmioBus.write32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL, 0);
    }
    
    std::vector<uint32_t> fetch(std::size_t nbytes) {
        std::vector<uint32_t> data;
        data.reserve(nbytes);
        
        for(;;) {
            std::uint32_t sinkRamRP = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_RP_LOW); // read RP_LOW to see how many bytes have been read
            std::uint32_t sinkRamWP = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_WP_LOW); // read WP_LOW to see how many bytes have been written
            if ((sinkRamWP == sinkRamRP) || (data.size() >= nbytes)) break;
            std::uint32_t sinkDataBufferValue = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_DATA); // read from TraceRamSink to see the data
            // Append the 4 bytes of value to data vector in little-endian order
            data.push_back(sinkDataBufferValue);
        }
        return data;
    }
    private:
        MmioBus& mmioBus;
        uint32_t trTeBase; // base address for TraceEncoder
        uint32_t trFunnelBase; // base address for TraceFunnel
        uint32_t trRamSinkBase; // base address for TraceRamSink
    };
}