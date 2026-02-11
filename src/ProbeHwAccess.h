#pragma once
#include <cstdint>
#include "IHwAccess.h"
#include "MmioBus.h"

namespace tci {

// Adapter: makes MmioBus look like a probe (IHwAccess)
class ProbeHwAccess final : public IHwAccess { // final to indicate this is not meant to be subclassed further
public:
    explicit ProbeHwAccess(MmioBus& bus) : bus_(bus) {}

    void WriteMemory(std::uint32_t address, std::uint32_t value) override {
        bus_.write32(address, value);
    }

    std::uint32_t ReadMemory(std::uint32_t address) override {
        return bus_.read32(address);
    }

private:
    MmioBus& bus_;
};

} // namespace tci
