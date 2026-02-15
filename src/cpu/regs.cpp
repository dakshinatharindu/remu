#include <remu/cpu/regs.hpp>

namespace remu::cpu {

RegFile::RegFile() {
    reset();
}

void RegFile::reset() {
    x_.fill(0);
}

std::uint32_t RegFile::read(std::uint32_t reg) const {
    if (reg >= 32) return 0;
    return x_[reg];
}

void RegFile::write(std::uint32_t reg, std::uint32_t value) {
    if (reg == 0 || reg >= 32) return; // x0 hardwired to 0
    x_[reg] = value;
}

} // namespace remu::cpu
