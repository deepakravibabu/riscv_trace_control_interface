#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include "TraceBytesConnect.h"
#include "IMmioDevice.h"

static constexpr uint32_t TR_RAM_CONTROL = 0x00;
static constexpr uint32_t TR_RAM_WP_LOW = 0x04;
static constexpr uint32_t TR_RAM_RP_LOW = 0x08;
static constexpr uint32_t TR_RAM_DATA = 0x0C;

class TraceRamSink : public TraceBytesConnect, public IMmioDevice {
public:
    TraceRamSink() {
        // std::cout << "[TraceSink] constructor called" << std::endl;
    }
    
    ~TraceRamSink() {
        // std::cout << "[TraceRamSink] destructor called" << std::endl;
    }
    
    void pushBytes(const std::uint8_t* data, std::size_t length) override {
        if((trRamControl & 0x1u) == 0) {
            std::cout << "[TraceRamSink::pushBytes] Trace RAM sinking is disabled" << std::endl;
            return;
        }
        for (std::size_t i = 0; i < length; ++i) {
            dataBuffer.push_back(data[i]);
        }   
    }

    void printDataBuffer() {
        std::cout << "[TraceRamSink::printDataBuffer] Data buffer contents: ";
        for (const auto& byte : dataBuffer) {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // reset to decimal
    }

    std::uint32_t read32(std::uint32_t offset) override {
        switch (offset) {
            case TR_RAM_CONTROL:
                return trRamControl;
            case TR_RAM_WP_LOW:
                return trRamWPLow;
            case TR_RAM_RP_LOW:
                return trRamRPLow;
            case TR_RAM_DATA:
                return trRamData;
            default:
                std::cout << "[TraceRamSink::read32] Invalid offset: " << offset << std::endl;
                return 0;
        }
    }

    void write32(std::uint32_t offset, std::uint32_t value) override {
        switch (offset) {
            case TR_RAM_CONTROL:
                trRamControl = value;
                break;
            case TR_RAM_WP_LOW:
                trRamWPLow = value;
                break;
            case TR_RAM_RP_LOW:
                trRamRPLow = value;
                break;
            case TR_RAM_DATA:
                trRamData = value;
                break;
            default:
                std::cout << "[TraceRamSink::write32] Invalid offset: " << offset << std::endl;
                break;
        }
    }

private:
    std::vector<std::uint8_t> dataBuffer;

    std::uint32_t trRamControl = 0; // enable = 0 (default)
    std::uint32_t trRamWPLow = 0; 
    std::uint32_t trRamRPLow = 0; 
    std::uint32_t trRamData = 0;

};

