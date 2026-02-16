#include <remu/runtime/sim.hpp>

#include <remu/common/log.hpp>
#include <remu/cpu/decode.hpp>
#include <remu/cpu/execute.hpp>
#include <remu/mem/bus.hpp>

namespace remu::runtime {

Sim::Sim(remu::platform::VirtMachine& machine,
         remu::cpu::Cpu& cpu,
         const Arguments& opts)
    : machine_(machine), cpu_(cpu), opts_(opts) {}

bool Sim::fetch32_(std::uint32_t addr, std::uint32_t& out) {
    return machine_.bus().read32(addr, out);
}

// namespace {
// constexpr std::uint32_t MSTATUS_MIE   = 1u << 3;
// constexpr std::uint32_t MSTATUS_MPIE  = 1u << 7;
// constexpr std::uint32_t MSTATUS_MPP_MASK = 3u << 11;

// constexpr std::uint32_t MIE_MTIE = 1u << 7;
// constexpr std::uint32_t MIP_MTIP = 1u << 7;

// constexpr std::uint32_t MCAUSE_MTI = 0x80000007u; // interrupt + machine timer
// }

namespace {
constexpr std::uint32_t MSTATUS_MIE      = 1u << 3;
constexpr std::uint32_t MSTATUS_MPIE     = 1u << 7;
constexpr std::uint32_t MSTATUS_MPP_MASK = 3u << 11;

constexpr std::uint32_t MSTATUS_SIE      = 1u << 1;
constexpr std::uint32_t MSTATUS_SPIE     = 1u << 5;
constexpr std::uint32_t MSTATUS_SPP      = 1u << 8;

inline void enter_trap_m(remu::cpu::Cpu& cpu, std::uint32_t cause, std::uint32_t tval) {
    cpu.csr.set_mepc(cpu.pc);
    cpu.csr.set_mcause(cause);
    cpu.csr.set_mtval(tval);

    std::uint32_t ms = cpu.csr.mstatus();
    const bool prev_mie = (ms & MSTATUS_MIE) != 0;

    if (prev_mie) ms |= MSTATUS_MPIE;
    else          ms &= ~MSTATUS_MPIE;

    ms &= ~MSTATUS_MIE;

    const std::uint32_t mpp = static_cast<std::uint32_t>(cpu.priv) & 0x3u;
    ms = (ms & ~MSTATUS_MPP_MASK) | (mpp << 11);

    cpu.csr.write(0x300, ms);
    cpu.priv = remu::cpu::PrivMode::Machine;

    cpu.pc = cpu.csr.mtvec() & ~0x3u;
}

inline void enter_trap_s(remu::cpu::Cpu& cpu, std::uint32_t cause, std::uint32_t tval) {
    cpu.csr.set_sepc(cpu.pc);
    cpu.csr.set_scause(cause);
    cpu.csr.set_stval(tval);

    std::uint32_t ms = cpu.csr.mstatus();
    const bool prev_sie = (ms & MSTATUS_SIE) != 0;

    if (prev_sie) ms |= MSTATUS_SPIE;
    else          ms &= ~MSTATUS_SPIE;

    ms &= ~MSTATUS_SIE;

    // SPP = 1 if previous was S, else 0
    if (cpu.priv == remu::cpu::PrivMode::Supervisor) ms |= MSTATUS_SPP;
    else                                              ms &= ~MSTATUS_SPP;

    cpu.csr.write(0x300, ms);
    cpu.priv = remu::cpu::PrivMode::Supervisor;

    cpu.pc = cpu.csr.stvec() & ~0x3u;
}
} // namespace

bool Sim::step() {
    if (stop_reason_ != StopReason::None) return false;

    // const std::uint32_t mstatus = cpu_.csr.mstatus();
    // const std::uint32_t mie     = cpu_.csr.mie();
    // const std::uint32_t mip     = cpu_.csr.mip();

    // const bool take_mti =
    //     (mstatus & MSTATUS_MIE) &&
    //     (mie     & MIE_MTIE) &&
    //     (mip     & MIP_MTIP);

    // if (take_mti) {
    //     // Trap entry
    //     cpu_.csr.set_mepc(cpu_.pc);
    //     cpu_.csr.set_mcause(MCAUSE_MTI);
    //     cpu_.csr.set_mtval(0);

    //     // mstatus updates: MPIE <- MIE, MIE <- 0, MPP <- current priv
    //     std::uint32_t new_ms = mstatus;
    //     const std::uint32_t prev_mie = (new_ms & MSTATUS_MIE) ? 1u : 0u;

    //     if (prev_mie) new_ms |= MSTATUS_MPIE;
    //     else          new_ms &= ~MSTATUS_MPIE;

    //     new_ms &= ~MSTATUS_MIE;

    //     // Set MPP = current priv (you likely run in Machine already)
    //     new_ms = (new_ms & ~MSTATUS_MPP_MASK) | (3u << 11); // Machine=3
    //     cpu_.csr.write(0x300, new_ms); // or cpu_.csr.set_mstatus(new_ms) if you have it

    //     cpu_.priv = remu::cpu::PrivMode::Machine;

    //     // mtvec direct mode
    //     cpu_.pc = cpu_.csr.mtvec() & ~0x3u;

    //     // Still tick time forward
    //     machine_.tick(1, cpu_);
    //     cpu_.csr.increment_cycle(1);
    //     return true;
    // }


    // interrupt pending bits
    constexpr std::uint32_t MIP_MTIP = 1u << 7;
    constexpr std::uint32_t MIE_MTIE = 1u << 7;

    // For supervisor timer interrupt, bit is STIP/STIE = 1<<5
    constexpr std::uint32_t SIP_STIP = 1u << 5;
    constexpr std::uint32_t SIE_STIE = 1u << 5;

    // Interrupt causes (lower bits of cause)
    constexpr std::uint32_t CAUSE_SUPERVISOR_TIMER = 5;
    constexpr std::uint32_t CAUSE_MACHINE_TIMER    = 7;
    constexpr std::uint32_t CAUSE_INT_FLAG         = 0x80000000u;

    const std::uint32_t ms = cpu_.csr.mstatus();
    const std::uint32_t mie = cpu_.csr.mie();
    const std::uint32_t mip = cpu_.csr.mip();

    // If MTIP is pending, decide whether it is delegated to S-mode
    const bool mt_pending = (mip & MIP_MTIP) != 0;
    const bool mt_enabled_m = (mie & MIE_MTIE) != 0; // Linux may still set it
    const bool delegated_timer = (cpu_.priv != remu::cpu::PrivMode::Machine) &&
                                ((cpu_.csr.mideleg() >> CAUSE_MACHINE_TIMER) & 1u);

    // If delegated, it appears as STIP/STIE from S-mode perspective.
    const bool sie = (ms & MSTATUS_SIE) != 0;
    const bool mie_g = (ms & MSTATUS_MIE) != 0;

    if (mt_pending) {
        if (delegated_timer) {
            // take S-mode timer interrupt if SIE enabled
            if (sie) {
                enter_trap_s(cpu_, CAUSE_INT_FLAG | CAUSE_SUPERVISOR_TIMER, 0);
                machine_.tick(1, cpu_);
                cpu_.csr.increment_cycle(1);
                return true;
            }
        } else {
            // take M-mode timer interrupt if MIE enabled
            if (mie_g && mt_enabled_m) {
                enter_trap_m(cpu_, CAUSE_INT_FLAG | CAUSE_MACHINE_TIMER, 0);
                machine_.tick(1, cpu_);
                cpu_.csr.increment_cycle(1);
                return true;
            }
        }
    }

    const std::uint32_t pc = cpu_.pc;

    // 1) Fetch
    std::uint32_t insn = 0;
    if (!fetch32_(pc, insn)) {
        stop_reason_ = StopReason::BusFaultFetch;
        return false;
    }

    // 2) Decode
    const remu::cpu::DecodedInsn d = remu::cpu::decode_rv32(insn);
    if (d.kind == remu::cpu::InsnKind::Illegal) {
        stop_reason_ = StopReason::IllegalInstruction;
        return false;
    }

    // --- synchronous traps ---
    if (d.kind == remu::cpu::InsnKind::ECALL) {
        std::uint32_t cause = 11; // ECALL from M
        if (cpu_.priv == remu::cpu::PrivMode::User)       cause = 8;  // from U
        else if (cpu_.priv == remu::cpu::PrivMode::Supervisor) cause = 9;  // from S

        const bool delegated =
            (cpu_.priv != remu::cpu::PrivMode::Machine) &&
            ((cpu_.csr.medeleg() >> cause) & 1u);

        if (delegated) enter_trap_s(cpu_, cause, 0);
        else           enter_trap_m(cpu_, cause, 0);

        instructions_ += 1;
        cpu_.csr.increment_instret(1);
        cpu_.csr.increment_cycle(1);
        machine_.tick(1, cpu_);
        return true;
    }

    if (d.kind == remu::cpu::InsnKind::EBREAK) {
        const std::uint32_t cause = 3; // breakpoint

        const bool delegated =
            (cpu_.priv != remu::cpu::PrivMode::Machine) &&
            ((cpu_.csr.medeleg() >> cause) & 1u);

        if (delegated) enter_trap_s(cpu_, cause, 0);
        else           enter_trap_m(cpu_, cause, 0);

        instructions_ += 1;
        cpu_.csr.increment_instret(1);
        cpu_.csr.increment_cycle(1);
        machine_.tick(1, cpu_);
        return true;
    }

    if (d.kind == remu::cpu::InsnKind::WFI) {
        // Advance PC like a normal instruction
        cpu_.pc += d.length;

        // If an interrupt is already deliverable, just return;
        // the next step() will take it in your interrupt-check code.
        auto interrupt_deliverable = [&]() -> bool {
            // Use your existing interrupt check logic:
            // - consider delegation (mideleg) and global enable bits
            // - check pending bits (mip) and enables (mie/sie)
            //
            // Minimal heuristic that usually works:
            const std::uint32_t ms = cpu_.csr.mstatus();
            const std::uint32_t mie = cpu_.csr.mie();
            const std::uint32_t mip = cpu_.csr.mip();

            constexpr std::uint32_t MSTATUS_MIE = 1u << 3;
            constexpr std::uint32_t MSTATUS_SIE = 1u << 1;

            // Pending sources
            constexpr std::uint32_t MIP_MSIP = 1u << 3;
            constexpr std::uint32_t MIP_MTIP = 1u << 7;
            constexpr std::uint32_t MIP_MEIP = 1u << 11;

            // Enables (machine)
            constexpr std::uint32_t MIE_MSIE = 1u << 3;
            constexpr std::uint32_t MIE_MTIE = 1u << 7;
            constexpr std::uint32_t MIE_MEIE = 1u << 11;

            const bool mie_g = (ms & MSTATUS_MIE) != 0;
            const bool sie_g = (ms & MSTATUS_SIE) != 0;

            // This is a simplification: if you already implemented delegation-aware
            // interrupt entry in Sim, you can call the same helper logic instead.
            const bool any_pending_enabled_m =
                mie_g &&
                ((mip & MIP_MSIP) && (mie & MIE_MSIE) ||
                (mip & MIP_MTIP) && (mie & MIE_MTIE) ||
                (mip & MIP_MEIP) && (mie & MIE_MEIE));

            // If SIE is set, kernel likely delegated interrupts; pending bits still
            // show up in mip and will be handled by your delegation logic.
            const bool any_pending_s = sie_g && (mip != 0);

            return any_pending_enabled_m || any_pending_s;
        };

        if (!interrupt_deliverable()) {
            // Sleep: advance time until something becomes deliverable.
            // Choose a reasonable step to not be too slow.
            for (int i = 0; i < 100000; ++i) {
                machine_.tick(1, cpu_);
                cpu_.csr.increment_cycle(1);

                if (interrupt_deliverable()) break;
            }
        }

        instructions_ += 1;
        cpu_.csr.increment_instret(1);
        return true;
    }



    // Optional trace hook
#ifdef REMU_ENABLE_TRACE
    remu::common::log_debug("pc=0x" + std::to_string(pc) + " insn=0x" + std::to_string(insn));
#endif

    // 3) Execute
    const bool ok = remu::cpu::execute(d, cpu_, machine_.bus());
    if (!ok) {
        // In our execute_rv32i(), we currently return false for ECALL/EBREAK too.
        // You can refine later by adding trap handling.
         if (d.kind == remu::cpu::InsnKind::ECALL || d.kind == remu::cpu::InsnKind::EBREAK) {
            stop_reason_ = StopReason::EcallOrEbreak;
        } else {
            stop_reason_ = StopReason::ExecuteFailed;
        }
        return false;
    }

    // 4) Accounting / ticks
    instructions_ += 1;
    cpu_.csr.increment_instret(1);
    cpu_.csr.increment_cycle(1);

    // Tick platform devices (timer increments, etc.)
    machine_.tick(1, cpu_);

    return true;
}

RunResult Sim::run(std::uint64_t max_instructions) {
    stop_reason_ = StopReason::None;
    instructions_ = 0;

    // Use override if provided, else default to "no limit" for now.
    const std::uint64_t limit = (max_instructions != 0) ? max_instructions : 0;

    while (true) {
        if (limit != 0 && instructions_ >= limit) {
            stop_reason_ = StopReason::InstructionLimit;
            break;
        }
        if (!step()) break;
    }

    RunResult rr;
    rr.reason = stop_reason_;
    rr.instructions = instructions_;
    rr.last_pc = cpu_.pc;
    return rr;
}

} // namespace remu::runtime
