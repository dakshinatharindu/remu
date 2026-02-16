#pragma once

#include <cstdint>
#include <mutex>

#include <remu/mem/region.hpp> // remu::mem::MmioDevice

namespace remu::devices {

// Minimal single-hart CLINT for QEMU virt style map.
// - msip[0] at offset 0x0000 (32-bit)
// - mtimecmp[0] at offset 0x4000 (64-bit split into 0x4000/0x4004)
// - mtime at offset 0xBFF8 (64-bit split into 0xBFF8/0xBFFC)
class Clint final : public remu::mem::MmioDevice {
public:
    Clint();

    // MmioDevice interface
    bool read (std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t& out) override;
    bool write(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t  val) override;

    // Advance time
    void tick(std::uint64_t cycles);

    // Query pending interrupts for hart0
    bool msip_pending() const;
    bool mtip_pending() const;

    // Optional: direct access for debug
    std::uint64_t mtime() const;
    std::uint64_t mtimecmp() const;

private:
    static std::uint32_t off_(std::uint32_t addr) {
        // CLINT mapped size is usually 0x10000; virt base ends with ...0000
        return (addr & 0xFFFFu);
    }

private:
    mutable std::mutex mu_;

    std::uint32_t msip0_ = 0;     // bit0 used
    std::uint64_t mtimecmp0_ = ~0ull; // default: never fire
    std::uint64_t mtime_ = 0;
};

} // namespace remu::devices
