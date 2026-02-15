#include <remu/mem/memory.hpp>

namespace remu::mem {

Memory::Memory(std::uint32_t base, std::uint32_t size_bytes)
    : base_(base), size_(size_bytes), data_(size_bytes, 0) {}

std::span<std::uint8_t> Memory::bytes() { return data_; }
std::span<const std::uint8_t> Memory::bytes() const { return data_; }

bool Memory::check_range_(std::uint32_t paddr, std::uint32_t len) const {
    if (paddr < base_) return false;
    const std::uint64_t off = static_cast<std::uint64_t>(paddr - base_);
    const std::uint64_t end = off + len;
    return end <= static_cast<std::uint64_t>(size_);
}

std::size_t Memory::index_(std::uint32_t paddr) const {
    return static_cast<std::size_t>(paddr - base_);
}

bool Memory::read8(std::uint32_t paddr, std::uint8_t& out) const {
    if (!check_range_(paddr, 1)) return false;
    out = data_[index_(paddr)];
    return true;
}

bool Memory::write8(std::uint32_t paddr, std::uint8_t val) {
    if (!check_range_(paddr, 1)) return false;
    data_[index_(paddr)] = val;
    return true;
}

bool Memory::read16(std::uint32_t paddr, std::uint16_t& out) const {
    if (!check_range_(paddr, 2)) return false;
    const auto i = index_(paddr);
    out = static_cast<std::uint16_t>(data_[i]) |
          (static_cast<std::uint16_t>(data_[i + 1]) << 8);
    return true;
}

bool Memory::write16(std::uint32_t paddr, std::uint16_t val) {
    if (!check_range_(paddr, 2)) return false;
    const auto i = index_(paddr);
    data_[i] = static_cast<std::uint8_t>(val & 0xFF);
    data_[i + 1] = static_cast<std::uint8_t>((val >> 8) & 0xFF);
    return true;
}

bool Memory::read32(std::uint32_t paddr, std::uint32_t& out) const {
    if (!check_range_(paddr, 4)) return false;
    const auto i = index_(paddr);
    out = static_cast<std::uint32_t>(data_[i]) |
          (static_cast<std::uint32_t>(data_[i + 1]) << 8) |
          (static_cast<std::uint32_t>(data_[i + 2]) << 16) |
          (static_cast<std::uint32_t>(data_[i + 3]) << 24);
    return true;
}

bool Memory::write32(std::uint32_t paddr, std::uint32_t val) {
    if (!check_range_(paddr, 4)) return false;
    const auto i = index_(paddr);
    data_[i] = static_cast<std::uint8_t>(val & 0xFF);
    data_[i + 1] = static_cast<std::uint8_t>((val >> 8) & 0xFF);
    data_[i + 2] = static_cast<std::uint8_t>((val >> 16) & 0xFF);
    data_[i + 3] = static_cast<std::uint8_t>((val >> 24) & 0xFF);
    return true;
}

}  // namespace remu::mem
