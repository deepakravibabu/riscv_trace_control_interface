#pragma once

#include <iostream>
#include <cstdint>
#include <cstddef>

#include "TraceBytesConnect.h"
#include "IMmioDevice.h"

static constexpr uint32_t TR_FUNNEL_CONTROL = 0x00;
static constexpr uint32_t TR_FUNNEL_DIS_INPUT = 0x04;

class TraceFunnel : public TraceBytesConnect, public IMmioDevice {
public:
    TraceFunnel() {
        // std::cout << "[TraceFunnel] constructor called" << std::endl;
    }

    ~TraceFunnel() {
        // std::cout << "[TraceFunnel] destructor called" << std::endl;
    }

    void connect(TraceBytesConnect* connector) {
        out = connector;
    }

    void pushBytes(const std::uint8_t* data, std::size_t length) override {
        if((trFunnelControl & 0x1u) == 0) {
            std::cout << "[TraceFunnel::pushBytes] Trace funneling is disabled" << std::endl;
            return;
        }
        if (out) {
            std::cout << "[TraceFunnel::pushBytes] Pushing bytes to connector" << std::endl;
            out->pushBytes(data, length);
        } else {
            std::cout << "[TraceFunnel::pushBytes] No out set" << std::endl;
        }
    }

    void set_funnelControl(uint32_t control) {
        trFunnelControl = control;
    }

    std::uint32_t read32(std::uint32_t offset) override {
        switch (offset) {
            case TR_FUNNEL_CONTROL:
                return trFunnelControl;
            case TR_FUNNEL_DIS_INPUT:
                return trFunnelDisInput;
            default:
                std::cout << "[TraceFunnel::read32] Invalid offset: " << offset << std::endl;
                return 0;
        }
    }

    void write32(std::uint32_t offset, std::uint32_t value) override {
        switch (offset) {
            case TR_FUNNEL_CONTROL:
                trFunnelControl = value;
                break;
            case TR_FUNNEL_DIS_INPUT:
                trFunnelDisInput = value;
                break;
            default:
                std::cout << "[TraceFunnel::write32] Invalid offset: " << offset << std::endl;
                break;
        }
    }
    
private:
    TraceBytesConnect* out = nullptr;

    std::uint32_t trFunnelControl = 0; // enable = 0 (default)
    std::uint32_t trFunnelDisInput = 0;

};

