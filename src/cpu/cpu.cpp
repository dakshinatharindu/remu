#include <remu/cpu/cpu.hpp>

namespace remu::cpu {

Cpu::Cpu() {
    reset(0);
}

void Cpu::reset(std::uint32_t reset_pc) {
    pc = reset_pc;
    priv = PrivMode::Machine;

    regs.reset();
    csr.reset();

    clear_reservation_();
}

void Cpu::set_boot_args(std::uint32_t a0_hartid, std::uint32_t a1_dtb_ptr) {
    regs.set_a0(a0_hartid);
    regs.set_a1(a1_dtb_ptr);
}

void Cpu::tick_counters(std::uint64_t cycles) {
    csr.increment_cycle(cycles);
    // Once you actually retire instructions, call increment_instret(1) per instruction.
}

void Cpu::clear_reservation_() {
    reservation_valid = false;
    reservation_addr = 0;
}

} // namespace remu::cpu
