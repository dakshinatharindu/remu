#pragma once

#include <cstdint>
#include <array>

namespace remu::cpu {

// Integer register file x0..x31 (RV32)
class RegFile {
public:
    RegFile();

    void reset();

    // Read x[rd]
    std::uint32_t read(std::uint32_t reg) const;

    // Write x[rd] (writes to x0 are ignored)
    void write(std::uint32_t reg, std::uint32_t value);

    // Convenience aliases (ABI names)
    std::uint32_t a0() const { return read(10); }
    std::uint32_t a1() const { return read(11); }
    void set_a0(std::uint32_t v) { write(10, v); }
    void set_a1(std::uint32_t v) { write(11, v); }

private:
    std::array<std::uint32_t, 32> x_;
};

} // namespace remu::cpu
