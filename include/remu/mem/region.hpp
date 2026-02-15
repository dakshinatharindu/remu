#pragma once

#include <cassert>
#include <cstdint>

namespace remu::mem {

class Memory;  // your direct RAM object (owns bytes)

// Simple MMIO device interface: devices implement this.
class MmioDevice {
   public:
    virtual ~MmioDevice() = default;

    // width_bytes is typically 1,2,4 (you can extend later)
    virtual bool read(std::uint32_t addr, std::uint32_t width_bytes,
                      std::uint32_t& out) = 0;
    virtual bool write(std::uint32_t addr, std::uint32_t width_bytes,
                       std::uint32_t val) = 0;
};

struct Region {
    enum class Kind { Ram, Mmio };

    std::uint32_t base = 0;
    std::uint32_t size = 0;
    Kind kind = Kind::Ram;

    Memory* ram = nullptr;       // valid when kind==Ram
    MmioDevice* mmio = nullptr;  // valid when kind==Mmio

    bool contains(std::uint32_t addr, std::uint32_t len = 1) const {
        // careful about overflow
        const std::uint64_t a = addr;
        const std::uint64_t b = base;
        const std::uint64_t end = a + len;
        return (a >= b) && (end <= (b + size));
    }

    static Region make_ram(std::uint32_t base, std::uint32_t size,
                           Memory* ram) {
        assert(ram != nullptr);
        return Region{base, size, Kind::Ram, ram, nullptr};
    }

    static Region make_mmio(std::uint32_t base, std::uint32_t size,
                            MmioDevice* dev) {
        assert(dev != nullptr);
        return Region{base, size, Kind::Mmio, nullptr, dev};
    }
};

}  // namespace remu::mem
