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
        sinkRamBufferSize(sinkRamBufferSize),         
        encoder(),
        funnel(),
        sink(sinkRamBufferSize),
        mmioBus()
    {
        // Connect the components
        encoder.connect(&funnel);
        funnel.connect(&sink);

        // Map all components via MMIOBus with 0x1000 size each (4 KB)
        mmioBus.addMapping(TR_TE_BASE, COMPONENT_SIZE, &encoder); // TraceEncoder at 0x1000 - 0x1FFF
        mmioBus.addMapping(TR_FUNNEL_BASE, COMPONENT_SIZE, &funnel); // TraceFunnel at 0x2000 - 0x2FFF
        mmioBus.addMapping(TR_RAM_SINK_BASE, COMPONENT_SIZE, &sink); // TraceRamSink at 0x3000 - 0x3FFF
    }

    public:
    void emitTrace(std::uint32_t pc, std::uint32_t opcode) {
        encoder.emitTrace(pc, opcode);
    }

    public:
    tci::MmioBus mmioBus;
    
    private:
    std::uint32_t sinkRamBufferSize; // default buffer size for TraceRamSink in bytes
    tci::TraceEncoder encoder;
    tci::TraceFunnel funnel;
    tci::TraceRamSink sink;

};