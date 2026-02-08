#pragma once
#include <cstdint>
#include <vector>

#include "MmioBus.h"
#include "TraceControlRegisters.h"


namespace tci {
    class TraceControllerInterface {
    public:
            TraceControllerInterface(MmioBus& bus, uint32_t teBase, uint32_t funnelBase, uint32_t ramSinkBase)
                : mmioBus(bus), trTeBase(teBase), trFunnelBase(funnelBase), trRamSinkBase(ramSinkBase) {}

        void configure();
        void start();
        void stop();
        std::vector<uint32_t> fetch(std::size_t nbytes);

    private:
        MmioBus& mmioBus;
        uint32_t trTeBase; // base address for TraceEncoder
        uint32_t trFunnelBase; // base address for TraceFunnel
        uint32_t trRamSinkBase; // base address for TraceRamSink
    };

    void TraceControllerInterface::configure() {
        // Enable TraceEncoder
        mmioBus.write32(trTeBase + TR_TE_CONTROL, 1);
        // Enable TraceFunnel
        mmioBus.write32(trFunnelBase + TR_FUNNEL_CONTROL, 1);
        mmioBus.write32(trFunnelBase + TR_FUNNEL_DIS_INPUT, 1); // enable funnel input
        // Enable TraceRamSink
        mmioBus.write32(trRamSinkBase + TR_RAM_CONTROL, 1);
    }

    void TraceControllerInterface::start() {
        // For this simple implementation, configure() already starts the tracing
        // In a more complex implementation, you might have separate control bits for starting/stopping
    }

    void TraceControllerInterface::stop() {
        // Disable Producer first to stop new data from being generated
        // Disable TraceEncoder
        mmioBus.write32(trTeBase + TR_TE_CONTROL, 0);
        // Disable TraceFunnel
        mmioBus.write32(trFunnelBase + TR_FUNNEL_CONTROL, 0);
        // Disable TraceRamSink
        mmioBus.write32(trRamSinkBase + TR_RAM_CONTROL, 0);
    }

    std::vector<uint32_t> TraceControllerInterface::fetch(std::size_t nbytes) {
        std::vector<uint32_t> data;
        data.reserve(nbytes);

        for(;;) {
            std::uint32_t sinkRamRP = mmioBus.read32(trRamSinkBase + TR_RAM_RP_LOW); // read RP_LOW to see how many bytes have been read
            std::uint32_t sinkRamWP = mmioBus.read32(trRamSinkBase + TR_RAM_WP_LOW); // read WP_LOW to see how many bytes have been written
            if ((sinkRamWP == sinkRamRP) || (data.size() >= nbytes)) break;
            std::uint32_t sinkDataBufferValue = mmioBus.read32(trRamSinkBase + TR_RAM_DATA); // read from TraceRamSink to see the data
            // Append the 4 bytes of value to data vector in little-endian order
            data.push_back(sinkDataBufferValue);
        }
        return data;
    }
    
}