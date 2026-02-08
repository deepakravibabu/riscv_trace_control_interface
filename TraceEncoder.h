#pragma once

#include <iostream>
#include <vector>
#include <cstdint>

#include "TraceBytesConnect.h"
#include "IMmioDevice.h"

static constexpr uint32_t TR_TE_CONTROL = 0x00;

class TraceEncoder : public IMmioDevice{
public:
    TraceEncoder() {
        // std::cout << "[TraceEncoder] constructor called" << std::endl;
    }

    ~TraceEncoder() {
        // std::cout << "[TraceEncoder] destructor called" << std::endl;
    }

    void connect(TraceBytesConnect* connector) {
        out = connector;
    }

    void emitInstr(){
        if((trTeControl & 0x1u) == 0) {
            std::cout << "[TraceEncoder::emitInstr] Trace encoding is disabled" << std::endl;
            return;
        }

        if (!out) {
            std::cout << "[TraceEncoder::emitInstr] No out set" << std::endl;
            return;
        }

        TraceBytes buffer; // using TraceBytes = std::vector<std::uint8_t>;
        buffer.reserve(8); // reserve space for 2 uint32_t
        append_u32_le(buffer, pcInstr[0]); // pc
        append_u32_le(buffer, pcInstr[1]); // opcode
        
        std::cout << "[TraceEncoder::emitInstr] buffer filled with pc and opcode" << std::endl;

        out->pushBytes(buffer);
    }

    void set_teControl(uint32_t control) {
        trTeControl = control;
    }

    std::uint32_t read32(std::uint32_t offset) override {
        switch (offset) {
            case TR_TE_CONTROL:
                return trTeControl;
            default:
                std::cout << "[TraceEncoder::read32] Invalid offset: " << offset << std::endl;
                return 0;
        }
    }

    void write32(uint32_t offset, uint32_t value) override {
        switch (offset) {
            case TR_TE_CONTROL:
                trTeControl = value;
                break;
            default:
                std::cout << "[TraceEncoder::write32] Invalid offset: " << offset << std::endl;
        }
    }


private:
    std::vector<uint32_t> pcInstr = {0x2000, 0xDEADBEEF}; // {pc, opcode}

    TraceBytesConnect* out = nullptr;
    std::uint32_t trTeControl = 0; // enable = 0 (default)

private:
    static void append_u32_le(std::vector<uint8_t>& buffer, uint32_t value) {
        buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
        buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
    }

};