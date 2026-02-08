#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "IMmioDevice.h"

namespace tci {
    struct MmioMapping {
        uint32_t baseAddress;
        uint32_t size;
        IMmioDevice* device;
    };

    class MmioBus {
        public:
        void addMapping(uint32_t baseAddress, uint32_t size, IMmioDevice* device) {
            mappings.push_back({baseAddress, size, device});
        }
        
        uint32_t read32(uint32_t address) {
            for (const auto& mapping : mappings) {
                if (address >= mapping.baseAddress && address < mapping.baseAddress + mapping.size) {
                    return mapping.device->read32(address - mapping.baseAddress);
                }
            }
            throw std::out_of_range("MMIO read out of range");
        }
        
        void write32(uint32_t address, uint32_t value) {
            for (const auto& mapping : mappings) {
                if (address >= mapping.baseAddress && address < mapping.baseAddress + mapping.size) {
                    mapping.device->write32(address - mapping.baseAddress, value); // global to local offset
                    return;
                }
            }
            throw std::out_of_range("MMIO write out of range");
        }

        private:
        std::vector<MmioMapping> mappings;
    };
}