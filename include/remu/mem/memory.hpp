#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace remu::mem {

class Memory {
   public:
    Memory(std::uint32_t base, std::uint32_t size_bytes);

    std::uint32_t base() const { return base_; }
    std::uint32_t size() const { return size_; }

    // Raw view (useful for fast loaders or debug dumps)
    std::span<std::uint8_t> bytes();
    std::span<const std::uint8_t> bytes() const;

    // Read/write helpers (little-endian)
    bool read8(std::uint32_t paddr, std::uint8_t& out) const;
    bool read16(std::uint32_t paddr, std::uint16_t& out) const;
    bool read32(std::uint32_t paddr, std::uint32_t& out) const;

    bool write8(std::uint32_t paddr, std::uint8_t val);
    bool write16(std::uint32_t paddr, std::uint16_t val);
    bool write32(std::uint32_t paddr, std::uint32_t val);

   private:
    bool check_range_(std::uint32_t paddr, std::uint32_t len) const;
    std::size_t index_(std::uint32_t paddr) const;

    std::uint32_t base_{0};
    std::uint32_t size_{0};
    std::vector<std::uint8_t> data_;
};

}  // namespace remu::mem
