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
    static void expectBits(uint32_t got, uint32_t mask, bool should_set) {
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
        // Configure trEncoderControl:
        // mmioBus.write32(trTeBase + tci::tr_te::TR_TE_CONTROL, 1);
        // enable trTeActive and trTeEnable, set trTeFormat to 0 (default)
        uint32_t trTeControlValue = tci::tr_te::TR_TE_ACTIVE |  tci::tr_te::TR_TE_INST_TRACING | ((0x5u << tci::tr_te::TR_TE_FORMAT_SHIFT) & tci::tr_te::TR_TE_FORMAT_MASK);
        mmioBus.write32(trTeBase + tci::tr_te::TR_TE_CONTROL, trTeControlValue);
        // Assertions to check the write
        uint32_t readBackValue = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL);
        expectBits(readBackValue, tci::tr_te::TR_TE_ACTIVE, true);
        expectBits(readBackValue, tci::tr_te::TR_TE_INST_TRACING, true);
        // Assertion
        uint32_t readBackFormat = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL);
        assert(bitFieldGet(readBackFormat, tci::tr_te::TR_TE_FORMAT_MASK, tci::tr_te::TR_TE_FORMAT_SHIFT) == 0x5u);
        // std::cout << "[TraceControllerInterface::configure] TraceEncoder configured with Active, InstTracing enabled and Format set to 0x5" << std::endl;
        
        // Configure trFunnelControl:
        // mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL, 1);
        // mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_DIS_INPUT, 1); // enable funnel input
        uint32_t trFunnelControlValue = tci::tr_tf::TR_FUNNEL_ACTIVE | tci::tr_tf::TR_FUNNEL_ENABLE;
        mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL, trFunnelControlValue);
        // Assertions to check the write
        uint32_t funnelReadBackValue = mmioBus.read32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL);
        expectBits(funnelReadBackValue, tci::tr_tf::TR_FUNNEL_ACTIVE, true);
        expectBits(funnelReadBackValue, tci::tr_tf::TR_FUNNEL_ENABLE, true);
        // std::cout << "[TraceControllerInterface::configure] TraceFunnel configured with Active and Enable set" << std::endl;

        // Configure trFunnelDisInput:
        uint32_t trFunnelDisInputValue = ( 0x0u & tci::tr_tf::TR_FUNNEL_DIS_INPUT_MASK); // 0 - enables all inputs to the funnel; 1 - disables
        mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_DIS_INPUT, trFunnelDisInputValue);
        // Assertion to check the write
        uint32_t funnelDisInputReadBackValue = mmioBus.read32(trFunnelBase + tci::tr_tf::TR_FUNNEL_DIS_INPUT);
        assert(bitFieldGet(funnelDisInputReadBackValue, tci::tr_tf::TR_FUNNEL_DIS_INPUT_MASK, 0) == 0x0u);
        // std::cout << "[TraceControllerInterface::configure] TraceFunnel input enabled (trFunnelDisInput set to 0)" << std::endl;
        

        // Configure trRamControl:
        // mmioBus.write32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL, 1);
        uint32_t trRamControlValue = tci::tr_ram::TR_RAM_ACTIVE | tci::tr_ram::TR_RAM_ENABLE;
        mmioBus.write32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL, trRamControlValue);
        // Assertions to check the write
        uint32_t ramReadBackValue = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL);
        expectBits(ramReadBackValue, tci::tr_ram::TR_RAM_ACTIVE, true);
        expectBits(ramReadBackValue, tci::tr_ram::TR_RAM_ENABLE, true);
        // std::cout << "[TraceControllerInterface::configure] TraceRamSink configured with Active and Enable set" << std::endl;

    }
    
    void start() {
        // Configure trEncoderControl to start producing trace data:
        // Read-Modify-Write to set the Enable bit while keeping other bits unchanged
        uint32_t trTeControlValue = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL) | tci::tr_te::TR_TE_ENABLE;
        mmioBus.write32(trTeBase + tci::tr_te::TR_TE_CONTROL, trTeControlValue);
        // Assertions to check the write
        uint32_t readBackValue = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL);
        expectBits(readBackValue, tci::tr_te::TR_TE_ENABLE, true);
        // std::cout << "[TraceControllerInterface::start] Trace production started by enabling TraceEncoder" << std::endl;

    }
    
    void stop() {
        // Disable Producer first to stop new data from being generated
        // Disable TraceEncoder
        uint32_t trTeControlValue = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL) & ~tci::tr_te::TR_TE_ENABLE;
        mmioBus.write32(trTeBase + tci::tr_te::TR_TE_CONTROL, trTeControlValue);
        // Assertions to check the write
        uint32_t readBackValue = mmioBus.read32(trTeBase + tci::tr_te::TR_TE_CONTROL);
        expectBits(readBackValue, tci::tr_te::TR_TE_ENABLE, false);

        // Disable TraceFunnel
        uint32_t trFunnelControlValue = mmioBus.read32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL) & ~tci::tr_tf::TR_FUNNEL_ENABLE;
        mmioBus.write32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL, trFunnelControlValue);
        // Assertions to check the write
        uint32_t funnelReadBackValue = mmioBus.read32(trFunnelBase + tci::tr_tf::TR_FUNNEL_CONTROL);
        expectBits(funnelReadBackValue, tci::tr_tf::TR_FUNNEL_ENABLE, false);

        // Disable TraceRamSink
        uint32_t trRamControlValue = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL) & ~tci::tr_ram::TR_RAM_ENABLE;
        mmioBus.write32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL, trRamControlValue);
        // Assertions to check the write
        uint32_t ramReadBackValue = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL);
        expectBits(ramReadBackValue, tci::tr_ram::TR_RAM_ENABLE, false);
    }
    
    std::vector<uint32_t> fetch(std::size_t wordCount) {
        std::vector<uint32_t> data;
        data.reserve(wordCount);

        while(data.size() < wordCount) {
            std::uint32_t sinkRamRP = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_RP_LOW); // read RP_LOW to see how many bytes have been read
            std::uint32_t sinkRamWP = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_WP_LOW); // read WP_LOW to see how many bytes have been written
            if ((sinkRamWP == sinkRamRP)) break; // no more data available
            std::uint32_t sinkDataBufferValue = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_DATA); // advances rp by 4 bytes.  Read from TraceRamSink to see the data
            data.push_back(sinkDataBufferValue);

            const std::uint32_t ctrl = mmioBus.read32(trRamSinkBase + tci::tr_ram::TR_RAM_CONTROL);
            const bool empty = (ctrl & tci::tr_ram::TR_RAM_EMPTY) != 0;
            if(empty) break; // no more data available
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