#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include "TraceBytesConnect.h"
#include "IMmioDevice.h"
#include "TraceControlRegisters.h"

// static constexpr uint32_t TR_RAM_CONTROL = 0x00;
// static constexpr uint32_t TR_RAM_WP_LOW = 0x04;
// static constexpr uint32_t TR_RAM_RP_LOW = 0x08;
// static constexpr uint32_t TR_RAM_DATA = 0x0C;

namespace tci {
    class TraceRamSink : public TraceBytesConnect, public IMmioDevice {
    public:
        TraceRamSink(std::size_t bufferSize = 32) : dataBuffer(bufferSize, 0) {
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
                // dataBuffer.push_back(data[i]);
                if(count == dataBuffer.size()) {
                    overflow = true;
                    std::cout << "[TraceRamSink::pushBytes] Data buffer overflow, cannot push more bytes" << std::endl;
                    return;
                }
                dataBuffer[trRamWPLow] = data[i];
                trRamWPLow = (trRamWPLow + 1) % dataBuffer.size();
                count++;
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
                    return pop_u32_le();
                default:
                    std::cout << "[TraceRamSink::read32] Invalid offset: " << offset << std::endl;
                    return 0;
            }
        }

        void write32(std::uint32_t offset, std::uint32_t value) override {
            switch (offset) {
                case TR_RAM_CONTROL:
                    if(value == 0) {
                        trRamControl = value;
                        resetDataBuffer();
                    } else if (value == 1) {
                        trRamControl = value;                    
                    } else {
                        std::cout << "[TraceRamSink::write32] Invalid control value: " << value << std::endl;
                        return;
                    }
                    break;
                case TR_RAM_WP_LOW:
                    // trRamWPLow = value; // ignore writes to WP_LOW
                    break;
                case TR_RAM_RP_LOW:
                    // trRamRPLow = value; // ignore writes to RP_LOW
                    break;
                case TR_RAM_DATA:
                    // trRamData = value; // ignore writes to DATA
                    break;
                default:
                    std::cout << "[TraceRamSink::write32] Invalid offset: " << offset << std::endl;
                    break;
            }
        }

    private:
        uint32_t pop_u32_le() {
            if(count < 4) {
                std::cout << "[TraceRamSink::pop_u32_le] Not enough data to pop a uint32_t" << std::endl;
                return 0;
            }
            uint32_t value = 0;
            for(int i = 0; i < 4; ++i) {
                value |= static_cast<uint32_t>(dataBuffer[trRamRPLow]) << (8 * i);
                trRamRPLow = (trRamRPLow + 1) % dataBuffer.size();
                count--;
            }
            return value;
        }

        void resetDataBuffer() {
            std::fill(dataBuffer.begin(), dataBuffer.end(), 0); // Clear the buffer
            trRamWPLow = 0;
            trRamRPLow = 0;
            count = 0;
            overflow = false;
        }

    private:
        std::uint32_t trRamControl = 0; // enable = 0 (default)
        std::vector<std::uint8_t> dataBuffer;
        std::uint32_t trRamWPLow = 0; 
        std::uint32_t trRamRPLow = 0; 
        std::uint32_t trRamData = 0;
        std::uint32_t count = 0;
        bool overflow = false;

    };
}
