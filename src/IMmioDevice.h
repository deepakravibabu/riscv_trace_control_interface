
#pragma once
#include <cstdint>

namespace tci {

    class IMmioDevice
    {
    public:
        virtual ~IMmioDevice() = default;

        // offset is within the device's address space, not the global address space
        virtual std::uint32_t read32(std::uint32_t offset) = 0;
        virtual void write32(std::uint32_t offset, std::uint32_t value) = 0;
    };

}