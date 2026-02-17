#pragma once

#include <cstdint>

#include <remu/cpu/regs.hpp>
#include <remu/cpu/csr.hpp>
#include <remu/cpu/exception.hpp>

namespace remu::cpu {

// Full hart state (RV32)
class Cpu {
public:
    Cpu();

    void reset(std::uint32_t reset_pc);

    // Architectural state
    std::uint32_t pc = 0;
    PrivMode priv = PrivMode::Machine;

    RegFile regs;
    CsrFile csr;

    // RV32A reservation (for LR/SC). Optional but useful for "A".
    bool reservation_valid = false;
    std::uint32_t reservation_addr = 0;

    // Linux boot convention helpers (a0/a1)
    void set_boot_args(std::uint32_t a0_hartid, std::uint32_t a1_dtb_ptr);

    // Called by the simulator each instruction (simple accounting)
    void tick_counters(std::uint64_t cycles = 1);

    // Pending synchronous exception (set by execute, consumed by trap)
    bool exception_pending = false;
    std::uint32_t exception_cause = 0;
    std::uint32_t exception_tval  = 0;

    void raise_exception(std::uint32_t cause, std::uint32_t tval = 0) {
        exception_pending = true;
        exception_cause = cause;
        exception_tval = tval;
    }

    void clear_pending_exception() {
        exception_pending = false;
        exception_cause = 0;
        exception_tval = 0;
    }

private:
    void clear_reservation_();
};

} // namespace remu::cpu
