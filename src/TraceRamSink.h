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
    TraceRamSink(std::uint32_t bufSize) : bufferSize_(bufSize) {
        dataBuffer_.resize(bufferSize_, 0);
        trRamControl_ = 0; // default control value with all bits cleared
        setEmpty(true); // initially empty
    }
    
    ~TraceRamSink() {
    }
        
    void pushBytes(const std::uint8_t* data, std::size_t length) override {
        const bool active = (trRamControl_ & tci::tr_ram::TR_RAM_ACTIVE) != 0;
        const bool enable = (trRamControl_ & tci::tr_ram::TR_RAM_ENABLE) != 0;

        if(!active || !enable) {
            std::cout << "[TraceRamSink::pushBytes] Trace RAM sinking is disabled" << std::endl;
            return;
        }

        // Policy: DROP-WHEN-FULL (no overwrite, no wrap modeling)
        for (std::size_t i = 0; i < length; ++i) {
            if (count_ == dataBuffer_.size()) {
                // Drop remaining bytes; do not modify pointers or count_.
                droppedBytes_ += static_cast<std::uint32_t>(length - i);
                break;
            }

            dataBuffer_[wpByte_] = data[i];
            wpByte_ = (wpByte_ + 1) % dataBuffer_.size();
            ++count_;

            // Any successful write means not empty anymore
            setEmpty(false);
        }
    }

    // void printDataBuffer() {
    //     std::cout << "[TraceRamSink::printDataBuffer] Data buffer contents: ";
    //     for (const auto& byte : dataBuffer_) {
    //         std::cout << std::hex << static_cast<int>(byte) << " ";
    //     }
    //     std::cout << std::dec << std::endl; // reset to decimal
    // }

    std::uint32_t read32(std::uint32_t offset) override {
        switch (offset) {
            case tci::tr_ram::TR_RAM_CONTROL:
                updateEmptyFromCount();
                return trRamControl_;
            case tci::tr_ram::TR_RAM_WP_LOW:
                // Simplified: WRAP bit[0] always reads 0; pointer in [31:2]
                return encodePtrAligned(wpByte_) & tci::tr_ram::TR_RAM_WP_LOW_MASK;
            case tci::tr_ram::TR_RAM_RP_LOW:
                return encodePtrAligned(rpByte_) & tci::tr_ram::TR_RAM_RP_LOW_MASK;
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
                const std::uint32_t oldValue = trRamControl_;

                const bool newActive = (value & tci::tr_ram::TR_RAM_ACTIVE) != 0;
                if(!newActive) {
                    trRamControl_ = 0; // reset all control bits to default values when deactivating
                    resetDataBuffer();
                    setEmpty(true); // set empty when deactivated
                    std::cout << "[TraceRamSink::write32] Trace RAM sinking deactivated, internal state reset, control bits cleared" << std::endl;
                    return;
                }

                // Normal masked write
                // keep RO bits(EMPTY) as oldValue
                const std::uint32_t keep_ro = oldValue & tci::tr_ram::TR_RAM_CONTROL_RO_MASK;
                // Take RW bits(ACTIVE, ENABLE, MODE, STOP_ON_WRAP, MEM_FORMAT, ASYNC_FREQ) from new value
                std::uint32_t new_rw  = value & tci::tr_ram::TR_RAM_CONTROL_RW_MASK;
                
                new_rw = normalizeWarlFields(new_rw);
                
                trRamControl_ = keep_ro | new_rw;

                // Internal status refresh
                updateEmptyFromCount();
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
    static std::uint32_t normalizeWarlFields(std::uint32_t rw_value) {
        // WARL-lite: clamp fields to legal bitwidth and (optionally) supported subset.

        auto clampField = [&](std::uint32_t mask, std::uint32_t shift, std::uint32_t max_val) {
            std::uint32_t v = (rw_value & mask) >> shift;
            if (v > max_val) v = max_val; // clamp
            rw_value = (rw_value & ~mask) | ((v << shift) & mask);
        };

        // trRamMemFormat is 2 bits [10:9] -> legal 0..3
        clampField(tci::tr_ram::TR_RAM_MEM_FORMAT_MASK,
                    tci::tr_ram::TR_RAM_MEM_FORMAT_SHIFT,
                    3u);

        // trRamAsyncFreq is 3 bits [14:12] -> legal 0..7
        clampField(tci::tr_ram::TR_RAM_ASYNC_FREQ_MASK,
                    tci::tr_ram::TR_RAM_ASYNC_FREQ_SHIFT,
                    7u);
        
        return rw_value;
    }

    // Align pointer to 4 bytes for register view ([31:2])
    static std::uint32_t encodePtrAligned(std::uint32_t byte_index) {
        return (byte_index & ~0x3u); // clear low 2 bits
    }

    void setEmpty(bool is_empty) {
        if (is_empty) trRamControl_ |= tci::tr_ram::TR_RAM_EMPTY;
        else          trRamControl_ &= ~tci::tr_ram::TR_RAM_EMPTY;
    }

    void updateEmptyFromCount() {
        setEmpty(count_ == 0);
    }

    private:
    // Pop one 32-bit word (little-endian) if available; advances RP by 4 bytes
    std::uint32_t pop_u32_le() {
        // Require 4 bytes to read a word
        if (count_ < 4 || dataBuffer_.empty()) {
            updateEmptyFromCount(); // do NOT set empty unless count_==0
            return 0;
        }

        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            value |= static_cast<std::uint32_t>(dataBuffer_[rpByte_]) << (8 * i);
            rpByte_ = (rpByte_ + 1) % dataBuffer_.size();
        }
        count_ -= 4;

        updateEmptyFromCount();
        return value;
    }

    // Optional: allow software to advance RP (consume) without reading DATA.
    // Forward-only; clamps to available data.
    // void apply_rp_write_forward_only(std::uint32_t value) {
    //     if (dataBuffer_.empty()) return;

    //     // Only consider aligned pointer bits [31:2]
    //     std::uint32_t new_ptr = value & tci::tr_ram::TR_RAM_RP_LOW_MASK;
    //     std::uint32_t new_rp = (new_ptr % static_cast<std::uint32_t>(dataBuffer_.size())) & ~0x3u;

    //     const std::uint32_t size = static_cast<std::uint32_t>(dataBuffer_.size());
    //     const std::uint32_t old_rp = rpByte_ & ~0x3u;

    //     // forward distance in ring
    //     std::uint32_t forward = 0;
    //     if (new_rp >= old_rp) forward = new_rp - old_rp;
    //     else forward = (size - old_rp) + new_rp;

    //     if (forward > count_) forward = count_; // cannot consume more than available

    //     rpByte_ = (rpByte_ + forward) % size;
    //     count_ -= forward;

    //     updateEmptyFromCount();
    // }

    void resetDataBuffer() {
        std::fill(dataBuffer_.begin(), dataBuffer_.end(), 0); // Clear the buffer
        wpByte_ = 0;
        rpByte_ = 0;
        count_ = 0;
    }

    private:
    std::uint32_t trRamControl_ = 0; // enable = 0 (default)
    std::vector<std::uint8_t> dataBuffer_;
    std::uint32_t bufferSize_ = 1024; // default buffer size in bytes (256 words)

    std::uint32_t count_ = 0;

    // Internal pointers are byte indices (0..size-1)
    // WP/RP are modeled as offsets within the sink’s internal buffer, not physical addresses.
    std::uint32_t wpByte_ = 0;
    std::uint32_t rpByte_ = 0;

    std::uint32_t droppedBytes_ = 0;

    };
}
