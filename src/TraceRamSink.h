#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include "TraceBytesConnect.h"
#include "IMmioDevice.h"
#include "TraceControlRegisters.h"


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

        // void printDataBuffer() {
        //     std::cout << "[TraceRamSink::printDataBuffer] Data buffer contents: ";
        //     for (const auto& byte : dataBuffer) {
        //         std::cout << std::hex << static_cast<int>(byte) << " ";
        //     }
        //     std::cout << std::dec << std::endl; // reset to decimal
        // }

        std::uint32_t read32(std::uint32_t offset) override {
            switch (offset) {
                case tci::tr_ram::TR_RAM_CONTROL:
                    return trRamControl;
                case tci::tr_ram::TR_RAM_WP_LOW:
                    return trRamWPLow;
                case tci::tr_ram::TR_RAM_RP_LOW:
                    return trRamRPLow;
                case tci::tr_ram::TR_RAM_DATA:
                    return pop_u32_le();
                default:
                    std::cout << "[TraceRamSink::read32] Invalid offset: " << offset << std::endl;
                    return 0;
            }
        }

        void write32(std::uint32_t offset, std::uint32_t value) override {
            switch (offset) {
                case tci::tr_ram::TR_RAM_CONTROL: {
                    const std::uint32_t oldValue = trRamControl;

                    const bool newActive = (value & tci::tr_ram::TR_RAM_ACTIVE) != 0;
                    if(!newActive) {
                        trRamControl = 0; // reset all control bits to default values when deactivating
                        resetDataBuffer();
                        std::cout << "[TraceRamSink::write32] Trace RAM sinking deactivated, internal state reset, control bits cleared" << std::endl;
                        return;
                    }

                    // Normal masked write
                    // keep RO bits(EMPTY) as oldValue
                    const std::uint32_t keep_ro = oldValue & tci::tr_ram::TR_RAM_CONTROL_RO_MASK;
                    // Take RW bits(ACTIVE, ENABLE, MODE, STOP_ON_WRAP, MEM_FORMAT, ASYNC_FREQ) from new value
                    std::uint32_t new_rw  = value & tci::tr_ram::TR_RAM_CONTROL_RW_MASK;
                    
                    new_rw = normalize_warl_fields(new_rw);
                    
                    trRamControl = keep_ro | new_rw;

                    break;
                }
                case tci::tr_ram::TR_RAM_WP_LOW:
                    // trRamWPLow = value; // ignore writes to WP_LOW
                    break;
                case tci::tr_ram::TR_RAM_RP_LOW:
                    // trRamRPLow = value; // ignore writes to RP_LOW
                    break;
                case tci::tr_ram::TR_RAM_DATA:
                    // trRamData = value; // ignore writes to DATA
                    break;
                default:
                    std::cout << "[TraceRamSink::write32] Invalid offset: " << offset << std::endl;
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

        // trRamMemFormat is 2 bits [10:9] -> legal 0..3
        clamp_field(tci::tr_ram::TR_RAM_MEM_FORMAT_MASK,
                    tci::tr_ram::TR_RAM_MEM_FORMAT_SHIFT,
                    3u);

        // trRamAsyncFreq is 3 bits [14:12] -> legal 0..7
        clamp_field(tci::tr_ram::TR_RAM_ASYNC_FREQ_MASK,
                    tci::tr_ram::TR_RAM_ASYNC_FREQ_SHIFT,
                    7u);
        
        return rw_value;
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
