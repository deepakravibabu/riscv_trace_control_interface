#pragma once
#include <cstdint>
#include <vector>
#include <iostream>
#include <cstring> // for strcmp

#include "IHwAccess.h"
#include "MmioBus.h"
#include "TraceControlRegisters.h" // for register offsets and bit definitions (deubg/logging purposes)

namespace tci {

// Adapter: makes MmioBus look like a probe (IHwAccess)
class ProbeHwAccess final : public IHwAccess { // final to indicate this is not meant to be subclassed further
public:
    struct ComponentRegion
    {
        uint32_t baseAddress;
        uint32_t size;
        const char* name; // for debug/logging purposes (TraceEncoder, TraceFunnel, TraceRamSink)
    };
    
    explicit ProbeHwAccess(MmioBus& bus, std::vector<ComponentRegion> regions) 
        : bus_(bus), componentRegions_(regions) {}

    void WriteMemory(std::uint32_t address, std::uint32_t value) override {
        // std::cout << "[ProbeHwAccess::WriteMemory] Writing value 0x" << std::hex << value 
        //             << " to address 0x" << address << std::dec << std::endl;

        auto info = decode(address);
        std::cout << "[PROBE WRITE]" << info.pretty << " <= 0x" << std::hex << value << std::dec << std::endl;
        bus_.write32(address, value);
    }

    std::uint32_t ReadMemory(std::uint32_t address) override {
        // std::cout << "[ProbeHwAccess::ReadMemory] Reading from address 0x" << std::hex << address 
        //             << std::dec << std::endl;

        auto info = decode(address);
        uint32_t value = bus_.read32(address);
        std::cout << "[PROBE READ]" << info.pretty << " => 0x" << std::hex << value << std::dec << std::endl;
        return value;
    }

private:
    struct DecodeInfo{
        const char* componentName = "Unknown";
        uint32_t base = 0;
        uint32_t offset = 0;
        const char* registerName = "Unknown";
        std::string pretty;
    };

    DecodeInfo decode(uint32_t address) {
        DecodeInfo info;
        // Component Range
        for(const auto& region : componentRegions_) {
            if(address >= region.baseAddress && address < region.baseAddress + region.size) {
                info.componentName = region.name;
                info.base = region.baseAddress;
                info.offset = address - region.baseAddress;
                break;
            }
        }

        // Register name based on offset (for known components)
        info.registerName = regName(info.componentName, info.offset);

        // Pretty string for logging
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "%s + 0x%03X (%s)", 
                      info.componentName, info.offset, info.registerName);
        info.pretty = buffer;

        return info;
    }

    static const char* regName(const char* componentName, uint32_t offset) {
        // This is a simplified example. In a real implementation, you would have a more comprehensive mapping.
        if (strcmp(componentName, "TraceEncoder") == 0) {
            switch (offset) {
                case tci::tr_te::TR_TE_CONTROL: return "TR_TE_CONTROL";
                // Add more TraceEncoder registers as needed
                default: return "Unknown Register";
            }
        } else if (strcmp(componentName, "TraceFunnel") == 0) {
            switch (offset) {
                case tci::tr_tf::TR_FUNNEL_CONTROL: return "TR_FUNNEL_CONTROL";
                case tci::tr_tf::TR_FUNNEL_DIS_INPUT: return "TR_FUNNEL_DIS_INPUT";
                // Add more TraceFunnel registers as needed
                default: return "Unknown Register";
            }
        } else if (strcmp(componentName, "TraceRamSink") == 0) {
            switch (offset) {
                case tci::tr_ram::TR_RAM_CONTROL: return "TR_RAM_CONTROL";
                case tci::tr_ram::TR_RAM_WP_LOW: return "TR_RAM_WP_LOW";
                case tci::tr_ram::TR_RAM_RP_LOW: return "TR_RAM_RP_LOW";
                case tci::tr_ram::TR_RAM_DATA: return "TR_RAM_DATA";
                // Add more TraceRamSink registers as needed
                default: return "Unknown Register";
            }
        }
        return "Unknown Component";
    }

private:
    MmioBus& bus_;
    std::vector<ComponentRegion> componentRegions_;
};

} // namespace tci
