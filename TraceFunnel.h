#pragma once

#include <iostream>
#include <cstdint>
#include <cstddef>

#include "TraceBytesConnect.h"

class TraceFunnel : public TraceBytesConnect {
    public:
    TraceFunnel() {
        std::cout << "[TraceFunnel] constructor called" << std::endl;
    }

    ~TraceFunnel() {
        std::cout << "[TraceFunnel] destructor called" << std::endl;
    }
    
    void connect(TraceBytesConnect* connector) {
        traceByteConnector = connector;
    }

    void pushBytes(const std::uint8_t* data, std::size_t length) override {
        std::cout << "[TraceFunnel::pushBytes] pushBytes method called" << std::endl;
        if(trFunnelControl != 1) {
            std::cout << "[TraceFunnel::pushBytes] Trace funneling is disabled" << std::endl;
            return;
        }
        if (traceByteConnector) {
            std::cout << "[TraceFunnel::pushBytes] Pushing bytes to connector" << std::endl;
            traceByteConnector->pushBytes(data, length);
        } else {
            std::cout << "[TraceFunnel::pushBytes] No traceByteConnector set" << std::endl;
        }
    }

    void set_funnelControl(uint32_t control) {
        trFunnelControl = control;
    }
    
    private:
        TraceBytesConnect* traceByteConnector = nullptr;

        std::uint32_t trFunnelControl = 1; // enable = 1 (default)
        std::uint32_t trFunnelDisInput = 0;

};

