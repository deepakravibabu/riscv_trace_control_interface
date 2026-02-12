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
    TraceRamSink(std::uint32_t bufSize) : bufferSize(bufSize) {
        dataBuffer.resize(bufferSize, 0);
        trRamControl = 0; // default control value with all bits cleared
        set_empty(true); // initially empty
    }
    
    ~TraceRamSink() {
    }
        
    void pushBytes(const std::uint8_t* data, std::size_t length) override {
        const bool active = (trRamControl & tci::tr_ram::TR_RAM_ACTIVE) != 0;
        const bool enable = (trRamControl & tci::tr_ram::TR_RAM_ENABLE) != 0;

        if(!active || !enable) {
            std::cout << "[TraceRamSink::pushBytes] Trace RAM sinking is disabled" << std::endl;
            return;
        }

        // Policy: DROP-WHEN-FULL (no overwrite, no wrap modeling)
        for (std::size_t i = 0; i < length; ++i) {
            if (count == dataBuffer.size()) {
                // Drop remaining bytes; do not modify pointers or count.
                dropped_bytes += static_cast<std::uint32_t>(length - i);
                break;
            }

            dataBuffer[wp_byte] = data[i];
            wp_byte = (wp_byte + 1) % dataBuffer.size();
            ++count;

            // Any successful write means not empty anymore
            set_empty(false);
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
                update_empty_from_count();
                return trRamControl;
            case tci::tr_ram::TR_RAM_WP_LOW:
                // Simplified: WRAP bit[0] always reads 0; pointer in [31:2]
                return encode_ptr_aligned(wp_byte) & tci::tr_ram::TR_RAM_WP_LOW_MASK;
            case tci::tr_ram::TR_RAM_RP_LOW:
                return encode_ptr_aligned(rp_byte) & tci::tr_ram::TR_RAM_RP_LOW_MASK;
            case tci::tr_ram::TR_RAM_DATA:
                return pop_u32_le(); // advances RP by 4 when successful
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
                    set_empty(true); // set empty when deactivated
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

                // Internal status refresh
                update_empty_from_count();
                break;
            }
            case tci::tr_ram::TR_RAM_WP_LOW:
                // ignore writes to WP_LOW
                break;
            case tci::tr_ram::TR_RAM_RP_LOW:
                // - SW may advance RP forward to consume data without reading DATA.
                // - Backward moves are ignored (ambiguous).
                // apply_rp_write_forward_only(value);
                break;
            case tci::tr_ram::TR_RAM_DATA:
                // read-only data port // ignore writes to DATA
                break;
            default:
                std::cout << "[TraceRamSink::write32] Invalid offset: " << offset << std::endl;
                break;
        }
    }

    private:
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

    // Align pointer to 4 bytes for register view ([31:2])
    static std::uint32_t encode_ptr_aligned(std::uint32_t byte_index) {
        return (byte_index & ~0x3u); // clear low 2 bits
    }

    void set_empty(bool is_empty) {
        if (is_empty) trRamControl |= tci::tr_ram::TR_RAM_EMPTY;
        else          trRamControl &= ~tci::tr_ram::TR_RAM_EMPTY;
    }

    void update_empty_from_count() {
        set_empty(count == 0);
    }

    private:
    // Pop one 32-bit word (little-endian) if available; advances RP by 4 bytes
    std::uint32_t pop_u32_le() {
        // Require 4 bytes to read a word
        if (count < 4 || dataBuffer.empty()) {
            update_empty_from_count(); // do NOT set empty unless count==0
            return 0;
        }

        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            value |= static_cast<std::uint32_t>(dataBuffer[rp_byte]) << (8 * i);
            rp_byte = (rp_byte + 1) % dataBuffer.size();
        }
        count -= 4;

        update_empty_from_count();
        return value;
    }

    // Optional: allow software to advance RP (consume) without reading DATA.
    // Forward-only; clamps to available data.
    // void apply_rp_write_forward_only(std::uint32_t value) {
    //     if (dataBuffer.empty()) return;

    //     // Only consider aligned pointer bits [31:2]
    //     std::uint32_t new_ptr = value & tci::tr_ram::TR_RAM_RP_LOW_MASK;
    //     std::uint32_t new_rp = (new_ptr % static_cast<std::uint32_t>(dataBuffer.size())) & ~0x3u;

    //     const std::uint32_t size = static_cast<std::uint32_t>(dataBuffer.size());
    //     const std::uint32_t old_rp = rp_byte & ~0x3u;

    //     // forward distance in ring
    //     std::uint32_t forward = 0;
    //     if (new_rp >= old_rp) forward = new_rp - old_rp;
    //     else forward = (size - old_rp) + new_rp;

    //     if (forward > count) forward = count; // cannot consume more than available

    //     rp_byte = (rp_byte + forward) % size;
    //     count -= forward;

    //     update_empty_from_count();
    // }

    void resetDataBuffer() {
        std::fill(dataBuffer.begin(), dataBuffer.end(), 0); // Clear the buffer
        wp_byte = 0;
        rp_byte = 0;
        count = 0;
    }

    private:
    std::uint32_t trRamControl = 0; // enable = 0 (default)
    std::vector<std::uint8_t> dataBuffer;
    std::uint32_t bufferSize = 1024; // default buffer size in bytes (256 words)

    std::uint32_t count = 0;

    // Internal pointers are byte indices (0..size-1)
    std::uint32_t wp_byte = 0;
    std::uint32_t rp_byte = 0;

    std::uint32_t dropped_bytes = 0;

    };
}
