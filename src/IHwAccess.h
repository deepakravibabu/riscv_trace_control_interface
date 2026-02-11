#pragma once
#include <cstdint>

namespace tci {

// Probe/tool hardware-access abstraction (only allowed access path for controller)
class IHwAccess {
public:
    virtual ~IHwAccess() = default;
    virtual void WriteMemory(std::uint32_t address, std::uint32_t value) = 0;
    virtual std::uint32_t ReadMemory(std::uint32_t address) = 0;
};

} // namespace tci
