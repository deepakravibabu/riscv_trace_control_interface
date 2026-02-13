#pragma once
#include <cstdint>
#include "traceEncoder.h"
#include "traceFunnel.h"
#include "traceRamSink.h"
#include "MmioBus.h"
#include <iostream>


class TraceSystem {
    public:
    static constexpr uint32_t TR_TE_BASE = 0x1000;
    static constexpr uint32_t TR_FUNNEL_BASE = 0x2000;
    static constexpr uint32_t TR_RAM_SINK_BASE = 0x3000;
    static constexpr uint32_t COMPONENT_SIZE = 0x1000; // 4 KB
    
    TraceSystem(std::uint32_t sinkRamBufferSize) : 
        sinkRamBufferSize_(sinkRamBufferSize),         
        encoder_(),
        funnel_(),
        sink_(sinkRamBufferSize_),
        mmioBus()
    {
        // Connect the components
        encoder_.connect(&funnel_);
        funnel_.connect(&sink_);

        // Map all components via MMIOBus with 0x1000 size each (4 KB)
        mmioBus.addMapping(TR_TE_BASE, COMPONENT_SIZE, &encoder_); // TraceEncoder at 0x1000 - 0x1FFF
        mmioBus.addMapping(TR_FUNNEL_BASE, COMPONENT_SIZE, &funnel_); // TraceFunnel at 0x2000 - 0x2FFF
        mmioBus.addMapping(TR_RAM_SINK_BASE, COMPONENT_SIZE, &sink_); // TraceRamSink at 0x3000 - 0x3FFF
    }

    public:
    void emitTrace(std::uint32_t pc, std::uint32_t opcode) {
        encoder_.emitTrace(pc, opcode);
    }

    public:
    tci::MmioBus mmioBus;
    
    private:
    std::uint32_t sinkRamBufferSize_; // default buffer size for TraceRamSink in bytes
    tci::TraceEncoder encoder_;
    tci::TraceFunnel funnel_;
    tci::TraceRamSink sink_;

};