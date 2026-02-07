#pragma once

#include <iostream>
#include <vector>
#include <cstdint>

#include "TraceBytesConnect.h"

class TraceEncoder {
    public:
    TraceEncoder() {
        std::cout << "[TraceEncoder] constructor called" << std::endl;
    }
    
    ~TraceEncoder() {
        std::cout << "[TraceEncoder] destructor called" << std::endl;
    }

    void connect(TraceBytesConnect* connector) {
        traceByteConnector = connector;
    }
    
    void emitInstr(){
        if(trTeControl != 1) {
            std::cout << "[TraceEncoder::emitInstr] Trace encoding is disabled" << std::endl;
            return;
        }

        if (!traceByteConnector) {
            std::cout << "[TraceEncoder::emitInstr] No traceByteConnector set" << std::endl;
            return;
        }

        TraceBytes buffer; // using TraceBytes = std::vector<std::uint8_t>;
        buffer.reserve(8); // reserve space for 2 uint32_t
        append_u32_le(buffer, pcInstr[0]); // pc
        append_u32_le(buffer, pcInstr[1]); // opcode
        
        std::cout << "buffer filled with pc and opcode" << std::endl;

        traceByteConnector->pushBytes(buffer);
    }

    void set_teControl(uint32_t control) {
        trTeControl = control;
    }
    
    private:
        std::vector<uint32_t> pcInstr = {0x1000, 0xDEADBEEF}; // {pc, opcode}

        TraceBytesConnect* traceByteConnector = nullptr;
        std::uint32_t trTeControl = 1; // enable = 1 (default)

    private:
        static void append_u32_le(std::vector<uint8_t>& buffer, uint32_t value) {
            buffer.push_back(value & 0xFF);
            buffer.push_back((value >> 8) & 0xFF);
            buffer.push_back((value >> 16) & 0xFF);
            buffer.push_back((value >> 24) & 0xFF);
        }

};