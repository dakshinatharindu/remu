#pragma once

#include <cstdint>
#include <vector>

#include <remu/mem/region.hpp>

namespace remu::mem {

class Bus {
public:
    // Map regions (call from platform/machine setup)
    void map_ram (std::uint32_t base, std::uint32_t size, Memory& ram);
    void map_mmio(std::uint32_t base, std::uint32_t size, MmioDevice& dev);

    // Loads/stores used by CPU + loaders
    bool read8 (std::uint32_t addr, std::uint8_t&  out);
    bool read16(std::uint32_t addr, std::uint16_t& out);
    bool read32(std::uint32_t addr, std::uint32_t& out);

    bool write8 (std::uint32_t addr, std::uint8_t  val);
    bool write16(std::uint32_t addr, std::uint16_t val);
    bool write32(std::uint32_t addr, std::uint32_t val);

private:
    Region*       find_region_(std::uint32_t addr, std::uint32_t len);
    const Region* find_region_(std::uint32_t addr, std::uint32_t len) const;

    // Helpers
    bool mmio_read_(MmioDevice& dev, std::uint32_t addr, std::uint32_t width, std::uint32_t& out);
    bool mmio_write_(MmioDevice& dev, std::uint32_t addr, std::uint32_t width, std::uint32_t val);

private:
    std::vector<Region> regions_;
};

} // namespace remu::mem
