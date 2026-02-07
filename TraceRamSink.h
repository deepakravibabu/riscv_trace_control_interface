#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include "TraceBytesConnect.h"

class TraceRamSink : public TraceBytesConnect {
    public:
        TraceRamSink() {
            std::cout << "[TraceSink] constructor called" << std::endl;
        }
        
        ~TraceRamSink() {
            std::cout << "[TraceRamSink] destructor called" << std::endl;
        }
        
        void pushBytes(const std::uint8_t* data, std::size_t length) override {
            std::cout << "[TraceRamSink::pushBytes] pushBytes method called" << std::endl;
            if(trRamControl != 1) {
                std::cout << "[TraceRamSink::pushBytes] Trace RAM sinking is disabled" << std::endl;
                return;
            }
            for (size_t i = 0; i < length; ++i) {
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

    private:
        std::vector<std::uint8_t> dataBuffer;

        std::uint32_t trRamControl = 1; // enable = 1 (default)
        std::uint32_t trRamWPLow = 0; 
        std::uint32_t trRamRPLow = 0; 
        std::uint32_t trRamData = 0;
    
};

